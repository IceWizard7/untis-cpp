#include "render.hpp"

#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <regex>
#include <stdexcept>
#include <thread>
#include <ixwebsocket/IXHttpClient.h>
#include <ixwebsocket/IXNetSystem.h>
#include <nlohmann/json.hpp>

#include "utils/html_capture.hpp"

// Helpers

namespace {
    constexpr int CDP_FETCH_ATTEMPTS = 60;
    constexpr auto CDP_FETCH_INTERVAL = std::chrono::milliseconds(500);
    constexpr auto WEBSOCKET_OPEN_TIMEOUT = std::chrono::seconds(10);
    constexpr auto FRAME_ID_TIMEOUT = std::chrono::seconds(10);
    constexpr auto RENDER_STEP_TIMEOUT = std::chrono::seconds(20);
}

str Renderer::get_websocket_url() {
    ix::HttpClient httpClient;

    int last_status = 0;

    for (int attempt = 0; attempt < CDP_FETCH_ATTEMPTS; ++attempt) {
        auto request = httpClient.createRequest();
        request->connectTimeout = 1;
        request->transferTimeout = 1;

        const auto response = httpClient.get("http://127.0.0.1:9222/json", request);
        if (response) {
            last_status = response->statusCode;

            if (response->statusCode == 200) {
                const auto targets = nlohmann::json::parse(response->body, nullptr, false);
                if (targets.is_array()) {
                    for (const auto &target : targets) {
                        // Recent Chrome versions also expose browser_ui targets (e.g. the macOS omnibox popup)
                        // They can accept HTML and screenshots but do not behave like a renderable page.
                        if (target.is_object() && target.value("type", str{}) == "page" &&
                            target.contains("webSocketDebuggerUrl") &&
                            target["webSocketDebuggerUrl"].is_string()) {
                            const auto url = target["webSocketDebuggerUrl"].get<str>();
                            if (!url.empty()) {
                                return url;
                            }
                        }
                    }
                }
            }
        }
        std::this_thread::sleep_for(CDP_FETCH_INTERVAL);
    }

    std::cerr << "No debuggable page at /json after waiting (HTTP " << last_status << "). Launch Chromium with about:blank.\n";
    return "";
}

str Renderer::escape_for_json(const str &s) {
    str out;
    out.reserve(s.size());
    for (const char c: s) {
        if (c == '"')
            out += "\\\"";
        else if (c == '\\')
            out += "\\\\";
        else if (c == '\n')
            out += "\\n";
        else if (c == '\r')
            out += "\\r";
        else
            out += c;
    }
    return out;
}

str Renderer::sanitize_filename(const str &name) {
    // Same as: re.sub(r'[^A-Za-z0-9\-_. ]+', '_', name)
    return std::regex_replace(name, std::regex("[^A-Za-z0-9\\-_. ]+"), "_");
}

std::vector<uint8_t> Renderer::base64_decode(const str &b64) {
    static constexpr auto T = []() {
        std::array<int, 256> t{};
        t.fill(-1);
        const auto chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        for (int i = 0; i < 64; ++i)
            t[static_cast<uint8_t>(chars[i])] = i;
        return t;
    }();

    std::vector<uint8_t> out;
    out.reserve(b64.size() * 3 / 4);

    int val = 0;
    int valb = -8;

    for (const unsigned char c: b64) {
        if (c == '=')
            break;
        if (T[c] == -1)
            continue;
        val = (val << 6) | T[c];
        valb += 6;
        if (valb >= 0) {
            out.push_back(static_cast<uint8_t>((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return out;
}

bool Renderer::write_base64_png(const str &base64, const str &filename) {
    // Strip optional data-URL prefix:  "data:image/png;base64,<data>"
    const str *src = &base64;
    str stripped;
    const auto comma = base64.find(',');
    if (base64.rfind("data:image", 0) == 0 && comma != str::npos) {
        stripped = base64.substr(comma + 1);
        src = &stripped;
    }

    const std::vector<uint8_t> bytes = base64_decode(*src);
    if (bytes.empty())
        return false;

    std::ofstream file(filename, std::ios::binary);
    if (!file)
        return false;

    file.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));

    return file.good();
}

str Renderer::msg(const str &body) {
    return "{\"id\":" + std::to_string(msg_id_++) + "," + body + "}";
}


Renderer::Renderer() = default;

Renderer::~Renderer() {
    ws_.stop();
}

bool Renderer::is_ready() const {
    return ready_.load() && ws_.getReadyState() == ix::ReadyState::Open && !frame_id_.empty();
}

// Setup

int Renderer::setup() {
    if (is_ready()) {
        return 0;
    }

    std::lock_guard<std::mutex> setup_lock(setup_mutex_);

    if (is_ready()) {
        return 0;
    }

    ix::initNetSystem();

    const str ws_url = get_websocket_url();
    if (ws_url.empty()) {
        return 1;
    }

    ws_.stop();
    ready_ = false;
    frame_id_.clear();
    page_loaded_ = false;
    got_layout_metrics_ = false;
    got_screenshot_ = false;
    screenshot_data_.clear();

    // Connecting
    ws_.setUrl(ws_url);

    ws_.setOnMessageCallback([this](const ix::WebSocketMessagePtr &m) {
        if (m->type != ix::WebSocketMessageType::Message)
            return;
        const str &s = m->str;

        if (s.find("frameTree") != str::npos) {
            const auto pos = s.find(R"("id":")");
            if (pos != str::npos)
                frame_id_ = s.substr(pos + 6, s.find('"', pos + 6) - (pos + 6));
        }

        if (s.find(R"("data":")") != str::npos) {
            {
                const auto pos = s.find(R"("data":")");
                const auto start = pos + 8;
                const auto end = s.find('"', start);
                std::lock_guard<std::mutex> lk(mutex_);
                screenshot_data_ = s.substr(start, end - start);
            }
            got_screenshot_ = true;
        }

        // Correct CDP structure: result -> result -> type/value
        if (s.find("\"result\":{\"result\":{\"type\":\"string\"") != str::npos) {
            const auto p = s.find(R"("value":")");
            if (p != str::npos) {
                const auto start = p + 9;

                // Unescape \" sequences to recover the inner JSON
                str rect_json;
                for (size_t i = start; i < s.size(); ++i) {
                    if (s[i] == '\\' && i + 1 < s.size()) {
                        rect_json += s[i + 1];
                        ++i;
                    } else if (s[i] == '"') {
                        break; // Real closing quote
                    } else {
                        rect_json += s[i];
                    }
                }

                auto extract = [&](const str &key) -> double {
                    auto kp = rect_json.find("\"" + key + "\":");
                    if (kp == str::npos)
                        return 0.0;
                    kp = rect_json.find_first_of("-0123456789.", kp + key.size() + 3);
                    if (kp == str::npos)
                        return 0.0;
                    return std::stod(rect_json.substr(kp, rect_json.find_first_not_of("-0123456789.", kp) - kp));
                };

                content_size_ = {extract("x"), extract("y"), extract("width"), extract("height")};
                got_layout_metrics_ = true;
            }
        }

        if (s.find("Page.loadEventFired") != str::npos) {
            page_loaded_ = true;
        }
    });

    ws_.start();
    const auto open_deadline = std::chrono::steady_clock::now() + WEBSOCKET_OPEN_TIMEOUT;
    while (ws_.getReadyState() != ix::ReadyState::Open) {
        if (std::chrono::steady_clock::now() >= open_deadline) {
            std::cerr << "Timed out connecting to Chromium websocket\n";
            ws_.stop();
            return 1;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    if (!ws_.send(R"({"id":1,"method":"Page.enable"})").success ||
        !ws_.send(R"({"id":2,"method":"Page.getFrameTree"})").success) {
        std::cerr << "Failed to initialize Chromium page over websocket\n";
        ws_.stop();
        return 1;
    }

    const auto frame_deadline = std::chrono::steady_clock::now() + FRAME_ID_TIMEOUT;
    while (frame_id_.empty()) {
        if (ws_.getReadyState() != ix::ReadyState::Open) {
            std::cerr << "Chromium websocket closed before frame tree was available\n";
            ws_.stop();
            return 1;
        }
        if (std::chrono::steady_clock::now() >= frame_deadline) {
            std::cerr << "Timed out waiting for Chromium frame tree\n";
            ws_.stop();
            return 1;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // Browser ready
    ready_ = true;
    return 0;
}

// Core Rendering

str Renderer::generate_base64_image(const str &html) {
    return generate_base64_image(html, 140, 200, 3.78, 3.0);
}

str Renderer::generate_base64_image(const str &html, const int width_mm, const int height_mm,
                                    const double px_per_mm,
                                    const double scale) {
    std::lock_guard<std::mutex> render_lock(render_mutex_);
    if (setup() != 0) {
        throw std::runtime_error("Chromium DevTools endpoint is not ready at http://127.0.0.1:9222/json");
    }

    const auto wait_for = [this](const std::atomic<bool> &flag, const str &operation) {
        const auto deadline = std::chrono::steady_clock::now() + RENDER_STEP_TIMEOUT;
        while (!flag.load()) {
            if (ws_.getReadyState() != ix::ReadyState::Open) {
                ready_ = false;
                throw std::runtime_error("Chromium websocket closed while waiting for " + operation);
            }
            if (std::chrono::steady_clock::now() >= deadline) {
                throw std::runtime_error("Timed out waiting for Chromium " + operation);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    };

    const int vp_w = static_cast<int>(std::round(width_mm * px_per_mm));
    const int vp_h = static_cast<int>(std::round(height_mm * px_per_mm));
    set_device_metrics(vp_w, vp_h, scale);

    const auto capture_html = Render_Utils::prepare_html_for_capture(html);

    page_loaded_ = false;
    if (!ws_.send(msg("\"method\":\"Page.setDocumentContent\",\"params\":{"
                      "\"frameId\":\"" +
                      frame_id_ +
                      "\","
                      "\"html\":\"" +

                      escape_for_json(capture_html) + "\"}")).success
    ) {
        ready_ = false;
        throw std::runtime_error("Failed to send Page.setDocumentContent to Chromium");

    }
    wait_for(page_loaded_, "page load event");

    got_layout_metrics_ = false;
    if (!ws_.send(msg(R"x("method":"Runtime.evaluate","params":{)x"
            R"x("expression":"JSON.stringify(document.body.getBoundingClientRect())",)x"
            R"x("returnByValue":true})x")).success) {
        ready_ = false;
        throw std::runtime_error("Failed to send Runtime.evaluate to Chromium");
    }
    wait_for(got_layout_metrics_, "layout metrics");

    // Build clip rectangle from real content size
    auto fmt = [](const double v) {
        return std::to_string(v);
    };
    const str clip = "\"x\":" + fmt(content_size_.x) +
                     ","
                     "\"y\":" +
                     fmt(content_size_.y) +
                     ","
                     "\"width\":" +
                     fmt(content_size_.width) +
                     ","
                     "\"height\":" +
                     fmt(content_size_.height) +
                     ","
                     "\"scale\":" +
                     fmt(1.0);

    got_screenshot_ = false;
    if (!ws_.send(msg("\"method\":\"Page.captureScreenshot\","
                      "\"params\":{"
                      "\"format\":\"png\","
                      "\"captureBeyondViewport\":true," // Escape the viewport box
                      "\"clip\":{" +
                      clip + "}}")).success) {
        ready_ = false;
        throw std::runtime_error("Failed to send Page.captureScreenshot to Chromium");;
    }
    wait_for(got_screenshot_, "screenshot capture");

    // Got screenshot

    std::lock_guard<std::mutex> lk(mutex_);
    return screenshot_data_;
}

void Renderer::set_device_metrics(const int width_px, const int height_px, const double scale) {
    if (setup() != 0) {
        throw std::runtime_error("Chromium DevTools endpoint is not ready at http://127.0.0.1:9222/json");
    }

    if (!ws_.send(msg("\"method\":\"Emulation.setDeviceMetricsOverride\","
                      "\"params\":{"
                      "\"width\":" +
                      std::to_string(width_px) +
                      ","
                      "\"height\":" +
                      std::to_string(height_px) +
                      ","
                      "\"deviceScaleFactor\":" +
                      std::to_string(scale) +
                      ","
                      "\"mobile\":false}")).success) {
        ready_ = false;
        throw std::runtime_error("Failed to send Emulation.setDeviceMetricsOverride to Chromium");
    }
}

#include "render.hpp"
#include "config.hpp"

#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <ixwebsocket/IXHttpClient.h>
#include <ixwebsocket/IXNetSystem.h>
#include <regex>
#include <thread>

// Helpers

str Renderer::get_websocket_url() {
    ix::HttpClient httpClient;
    const auto response = httpClient.get("http://127.0.0.1:9222/json", httpClient.createRequest());

    if (response->statusCode != 200) {
        std::cerr << "Failed to fetch /json: " << response->statusCode << "\n";
        return "";
    }

    const str& body = response->body;
    const auto pos = body.find("\"webSocketDebuggerUrl\"");
    if (pos == str::npos) { std::cerr << "No webSocketDebuggerUrl found\n"; return ""; }

    const auto start = body.find('"', body.find(':', pos) + 1) + 1;
    return body.substr(start, body.find('"', start) - start);
}

str Renderer::escape_for_json(const str& s) {
    str out;
    out.reserve(s.size());
    for (const char c : s) {
        if      (c == '"')  out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else                out += c;
    }
    return out;
}

str Renderer::sanitize_filename(const str& name) {
    // Same as: re.sub(r'[^A-Za-z0-9\-_. ]+', '_', name)
    return std::regex_replace(name, std::regex("[^A-Za-z0-9\\-_. ]+"), "_");
}

std::vector<uint8_t> Renderer::base64_decode(const str& b64)
{
    static constexpr auto T = []() {
        std::array<int, 256> t{};
        t.fill(-1);
        const char* chars =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        for (int i = 0; i < 64; ++i)
            t[static_cast<uint8_t>(chars[i])] = i;
        return t;
    }();

    std::vector<uint8_t> out;
    out.reserve(b64.size() * 3 / 4);

    int val  = 0;
    int valb = -8;

    for (unsigned char c : b64) {
        if (c == '=') break;
        if (T[c] == -1) continue;
        val   = (val << 6) | T[c];
        valb += 6;
        if (valb >= 0) {
            out.push_back(static_cast<uint8_t>((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return out;
}

bool Renderer::write_base64_png(const str& base64, const str& filename)
{
    // Strip optional data-URL prefix:  "data:image/png;base64,<data>"
    const str* src = &base64;
    str stripped;
    auto comma = base64.find(',');
    if (base64.rfind("data:image", 0) == 0 && comma != str::npos) {
        stripped = base64.substr(comma + 1);
        src = &stripped;
    }

    std::vector<uint8_t> bytes = base64_decode(*src);
    if (bytes.empty())
        return false;

    std::ofstream file(filename, std::ios::binary);
    if (!file)
        return false;

    file.write(reinterpret_cast<const char*>(bytes.data()),
               static_cast<std::streamsize>(bytes.size()));

    return file.good();
}

str Renderer::msg(const str& body) {
    return "{\"id\":" + std::to_string(msg_id_++) + "," + body + "}";
}


Renderer::Renderer() = default;

Renderer::~Renderer() = default;

// Setup

int Renderer::setup() {
    ix::initNetSystem();

    const str ws_url = get_websocket_url();
    if (ws_url.empty()) return 1;

    std::cout << "Connecting to: " << ws_url << "\n";
    ws_.setUrl(ws_url);

    ws_.setOnMessageCallback([this](const ix::WebSocketMessagePtr& m) {
        if (m->type != ix::WebSocketMessageType::Message) return;
        const str& s = m->str;

        if (s.find("frameTree") != str::npos) {
            const auto pos = s.find(R"("id":")");
            if (pos != str::npos)
                frame_id_ = s.substr(pos + 6, s.find('"', pos + 6) - (pos + 6));
        }

        if (s.find(R"("data":")") != str::npos) {
            {
                const auto pos   = s.find(R"("data":")");
                const auto start = pos + 8;
                const auto end   = s.find('"', start);
                std::lock_guard<std::mutex> lk(mutex_); screenshot_data_ = s.substr(start, end - start);
            }
            got_screenshot_ = true;
        }

        // Correct CDP structure: result -> result -> type/value
        if (s.find("\"result\":{\"result\":{\"type\":\"string\"") != str::npos) {
            auto p = s.find(R"("value":")");
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

                std::cout << "rect_json: " << rect_json << "\n"; // Temporary debug

                auto extract = [&](const str& key) -> double {
                    auto kp = rect_json.find("\"" + key + "\":");
                    if (kp == str::npos) return 0.0;
                    kp = rect_json.find_first_of("-0123456789.", kp + key.size() + 3);
                    if (kp == str::npos) return 0.0;
                    return std::stod(rect_json.substr(kp,
                        rect_json.find_first_not_of("-0123456789.", kp) - kp));
                };

                content_size_ = { extract("x"), extract("y"),
                                  extract("width"), extract("height") };
                got_layout_metrics_ = true;
            }
        }

        if (s.find("Page.loadEventFired") != str::npos) {
            page_loaded_ = true;
        }
    });

    ws_.start();
    while (ws_.getReadyState() != ix::ReadyState::Open)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

    ws_.send(R"({"id":1,"method":"Page.enable"})");
    ws_.send(R"({"id":2,"method":"Page.getFrameTree"})");

    while (frame_id_.empty())
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

    std::cout << "Browser ready (frameId: " << frame_id_ << ")\n";
    return 0;
}

// Core Rendering

str Renderer::generate_base64_image(const str& html) {
    return generate_base64_image(html, 140, 200, 3.78, 3.0);
}

str Renderer::generate_base64_image(const str& html, const int width_mm, const int height_mm, const double px_per_mm, const double scale) {
    const int vp_w = static_cast<int>(std::round(width_mm  * px_per_mm));
    const int vp_h = static_cast<int>(std::round(height_mm * px_per_mm));
    set_device_metrics(vp_w, vp_h, scale);

    page_loaded_ = false;
    ws_.send(msg(
        "\"method\":\"Page.setDocumentContent\",\"params\":{"
        "\"frameId\":\"" + frame_id_ + "\","
        "\"html\":\""    + escape_for_json(html) + "\"}"));

    while (!page_loaded_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    got_layout_metrics_ = false;
    ws_.send(msg(
        R"x("method":"Runtime.evaluate","params":{)x"
        R"x("expression":"JSON.stringify(document.body.getBoundingClientRect())",)x"
        R"x("returnByValue":true})x"
    ));
    while (!got_layout_metrics_)
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    std::cout << "Got layout metrics" << std::endl;

    // Build clip rectangle from real content size
    auto fmt = [](const double v){ return std::to_string(v); };
    const str clip =
        "\"x\":"      + fmt(content_size_.x)      + ","
        "\"y\":"      + fmt(content_size_.y)      + ","
        "\"width\":"  + fmt(content_size_.width)  + ","
        "\"height\":" + fmt(content_size_.height) + ","
        "\"scale\":"  + fmt(scale);

    got_screenshot_ = false;
    ws_.send(msg(
        "\"method\":\"Page.captureScreenshot\","
        "\"params\":{"
        "\"format\":\"png\","
        "\"captureBeyondViewport\":true,"   // Escape the viewport box
        "\"clip\":{" + clip + "}}"));

    while (!got_screenshot_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    std::cout << "Got screenshot" << std::endl;

    std::lock_guard<std::mutex> lk(mutex_);
    return screenshot_data_;
}

void Renderer::set_device_metrics(const int width_px, const int height_px, const double scale) {
    ws_.send(msg(
        "\"method\":\"Emulation.setDeviceMetricsOverride\","
        "\"params\":{"
        "\"width\":"             + std::to_string(width_px)  + ","
        "\"height\":"            + std::to_string(height_px) + ","
        "\"deviceScaleFactor\":" + std::to_string(scale)     + ","
        "\"mobile\":false}"));
}
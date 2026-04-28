#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <vector>
#include <ixwebsocket/IXWebSocket.h>

class Renderer {
public:
    Renderer();
    ~Renderer();

    // Must be called once before any rendering
    int setup();
    static std::string sanitize_filename(const std::string& name);
    static std::vector<uint8_t> base64_decode(const std::string& b64);
    static bool write_base64_png(const std::string& base64, const std::string& filename);
    std::string generate_base64_image(const std::string& html);
    std::string generate_base64_image(const std::string& html, int width_mm, int height_mm, double px_per_mm, double scale);

    void set_device_metrics(int width_px, int height_px, double scale);

private:
    static std::string get_websocket_url();
    static std::string escape_for_json(const std::string& s);
    std::string msg(const std::string& body);

    struct {
        double x, y, width, height;
    } content_size_ {};
    std::atomic<bool> got_layout_metrics_{false};
    ix::WebSocket        ws_;
    std::string          frame_id_;
    std::atomic<int>     msg_id_{10};
    std::mutex           mutex_;
    std::string          screenshot_data_;
    std::atomic<bool>    got_screenshot_{false};
    std::atomic<bool>    page_loaded_{false};
};

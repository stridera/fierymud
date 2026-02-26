#include "admin_server.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string_view>

#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

using json = nlohmann::json;
using namespace asio;
using namespace asio::ip;

// Admin server constants
namespace {
// Buffer sizes
constexpr size_t HTTP_REQUEST_BUFFER_SIZE = 8192;

// HTTP status codes
constexpr int HTTP_OK = 200;
constexpr int HTTP_BAD_REQUEST = 400;
constexpr int HTTP_UNAUTHORIZED = 401;
constexpr int HTTP_NOT_FOUND = 404;
constexpr int HTTP_INTERNAL_ERROR = 500;

// HTTP header parsing
constexpr std::string_view AUTH_BEARER_PREFIX = "Authorization: Bearer ";

bool is_loopback_address(std::string_view bind_address) {
    return bind_address == "127.0.0.1" || bind_address == "::1" || bind_address == "localhost";
}

bool is_truthy_env(std::string_view value) {
    std::string lowered(value);
    std::transform(lowered.begin(), lowered.end(), lowered.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return lowered == "1" || lowered == "true" || lowered == "yes" || lowered == "on";
}

bool secure_token_equals(std::string_view expected, std::string_view provided) {
    const size_t max_len = std::max(expected.size(), provided.size());
    unsigned char diff = static_cast<unsigned char>(expected.size() ^ provided.size());
    for (size_t i = 0; i < max_len; ++i) {
        unsigned char lhs = i < expected.size() ? static_cast<unsigned char>(expected[i]) : 0;
        unsigned char rhs = i < provided.size() ? static_cast<unsigned char>(provided[i]) : 0;
        diff |= static_cast<unsigned char>(lhs ^ rhs);
    }
    return diff == 0;
}

std::optional<std::string> extract_bearer_token(const std::string &request) {
    auto headers_end = request.find("\r\n\r\n");
    if (headers_end == std::string::npos) {
        return std::nullopt;
    }

    auto first_line_end = request.find("\r\n");
    if (first_line_end == std::string::npos || first_line_end >= headers_end) {
        return std::nullopt;
    }

    size_t line_start = first_line_end + 2;
    while (line_start < headers_end) {
        size_t line_end = request.find("\r\n", line_start);
        if (line_end == std::string::npos || line_end > headers_end) {
            break;
        }

        std::string_view header_line(request.data() + line_start, line_end - line_start);
        if (header_line.rfind(AUTH_BEARER_PREFIX, 0) == 0) {
            std::string token(header_line.substr(AUTH_BEARER_PREFIX.size()));
            while (!token.empty() && (token.back() == ' ' || token.back() == '\t')) {
                token.pop_back();
            }
            return token;
        }

        line_start = line_end + 2;
    }

    return std::nullopt;
}
} // namespace

namespace fierymud {

AdminServer::AdminServer(uint16_t port, const std::string &bind_address)
    : port_(port), bind_address_(bind_address), running_(false) {
    // Load authentication token from environment.
    const char *token_env = std::getenv("FIERYMUD_ADMIN_TOKEN");
    if (token_env && *token_env != '\0') {
        auth_token_ = token_env;
    } else {
        auth_token_ = "";
    }

    const char *allow_unauth_env = std::getenv("FIERYMUD_ALLOW_UNAUTH_ADMIN");
    allow_unauthenticated_local_ = allow_unauth_env && is_truthy_env(allow_unauth_env);
    if (const char *cors_origin_env = std::getenv("FIERYMUD_ADMIN_CORS_ORIGIN");
        cors_origin_env && *cors_origin_env != '\0') {
        cors_origin_ = cors_origin_env;
    }

    if (auth_token_.empty()) {
        if (allow_unauthenticated_local_) {
            spdlog::warn("FIERYMUD_ADMIN_TOKEN not set; unauthenticated admin API enabled for loopback only");
        } else {
            spdlog::warn("FIERYMUD_ADMIN_TOKEN not set; admin API will refuse to start");
        }
    }
}

AdminServer::~AdminServer() { stop(); }

void AdminServer::start() {
    if (running_.load()) {
        spdlog::warn("Admin server already running");
        return;
    }

    if (auth_token_.empty()) {
        if (!allow_unauthenticated_local_) {
            spdlog::error("Refusing to start admin server without FIERYMUD_ADMIN_TOKEN");
            return;
        }
        if (!is_loopback_address(bind_address_)) {
            spdlog::error("Refusing unauthenticated admin server on non-loopback address '{}'", bind_address_);
            return;
        }
    }

    running_.store(true);
    io_context_ = std::make_unique<io_context>();

    // Create acceptor
    acceptor_ =
        std::make_unique<tcp::acceptor>(*io_context_, tcp::endpoint(asio::ip::make_address(bind_address_), port_));
    acceptor_->set_option(socket_base::reuse_address(true));

    spdlog::info("Starting admin server on {}:{}", bind_address_, port_);

    // Start accepting connections
    start_accept();

    // Run io_context in background thread
    server_thread_ = std::make_unique<std::thread>([this]() {
        spdlog::info("Admin server listening on {}:{}", bind_address_, port_);
        io_context_->run();
        spdlog::debug("Admin server io_context exited");
    });
}

void AdminServer::stop() {
    if (!running_.exchange(false)) {
        return; // Already stopped
    }

    spdlog::info("Stopping admin server...");

    // Stop io_context - this cancels all pending async operations
    if (io_context_) {
        io_context_->stop();
    }

    // Join the thread - should exit quickly now that io_context is stopped
    if (server_thread_ && server_thread_->joinable()) {
        server_thread_->join();
    }

    // Clean up
    acceptor_.reset();
    io_context_.reset();

    spdlog::info("Admin server stopped");
}

void AdminServer::register_handler(const std::string &path, CommandHandler handler) {
    handlers_[path] = std::move(handler);
    spdlog::debug("Registered admin handler for path: {}", path);
}

void AdminServer::start_accept() {
    if (!running_.load() || !acceptor_) {
        return;
    }

    auto socket = std::make_shared<tcp::socket>(*io_context_);

    acceptor_->async_accept(*socket, [this, socket](const std::error_code &ec) {
        if (ec) {
            if (ec != asio::error::operation_aborted && running_.load()) {
                spdlog::error("Admin server accept error: {}", ec.message());
            }
            return; // Don't continue accepting on error
        }

        // Handle connection in the io_context thread (no need for separate thread)
        handle_connection(socket);

        // Accept next connection
        start_accept();
    });
}

void AdminServer::handle_connection(std::shared_ptr<tcp::socket> socket) {
    try {
        std::error_code ec;
        auto write_and_close = [&](const std::string &response) {
            asio::write(*socket, asio::buffer(response), ec);
            socket->shutdown(tcp::socket::shutdown_both, ec);
            socket->close(ec);
        };

        // Read HTTP request
        std::array<char, HTTP_REQUEST_BUFFER_SIZE> buffer;
        size_t bytes_read = socket->read_some(asio::buffer(buffer), ec);

        if (ec) {
            spdlog::error("Error reading from admin client: {}", ec.message());
            return;
        }

        std::string request(buffer.data(), bytes_read);
        std::string path, body;
        std::string method = parse_request(request, path, body);

        if (method.empty() || path.empty()) {
            write_and_close(build_response(HTTP_BAD_REQUEST, "application/json",
                                           R"({"error":"Bad Request","message":"Malformed HTTP request"})"));
            return;
        }

        spdlog::debug("Admin request: {} {}", method, path);

        // Check authentication if token is set
        if (!auth_token_.empty()) {
            auto provided_token = extract_bearer_token(request);
            if (!provided_token || !secure_token_equals(auth_token_, *provided_token)) {
                write_and_close(
                    build_response(HTTP_UNAUTHORIZED, "application/json",
                                   R"({"error":"Unauthorized","message":"Missing or invalid authentication token"})"));
                return;
            }
        } else if (!allow_unauthenticated_local_ || !is_loopback_address(bind_address_)) {
            // Defense in depth: start() should already block this.
            write_and_close(
                build_response(HTTP_UNAUTHORIZED, "application/json",
                               R"({"error":"Unauthorized","message":"Admin server requires authentication"})"));
            return;
        }

        // Route to handler: try exact match first, then longest prefix match
        auto handler_it = handlers_.find(path);
        if (handler_it == handlers_.end()) {
            // Longest prefix match for parameterized routes like /api/admin/room/30/1
            size_t best_len = 0;
            for (auto it = handlers_.begin(); it != handlers_.end(); ++it) {
                if (path.starts_with(it->first) && it->first.size() > best_len &&
                    (path.size() == it->first.size() || path[it->first.size()] == '/')) {
                    best_len = it->first.size();
                    handler_it = it;
                }
            }
        }
        if (handler_it != handlers_.end()) {
            try {
                std::string response_body = handler_it->second(path, body);
                std::string response = build_response(HTTP_OK, "application/json", response_body);
                write_and_close(response);
            } catch (const std::exception &e) {
                spdlog::error("Handler error for {}: {}", path, e.what());
                json error_json = {{"error", "Internal Server Error"}, {"message", e.what()}};
                std::string response = build_response(HTTP_INTERNAL_ERROR, "application/json", error_json.dump());
                write_and_close(response);
            }
        } else {
            json error_json = {{"error", "Not Found"},
                               {"message", fmt::format("No handler registered for path: {}", path)}};
            std::string response = build_response(HTTP_NOT_FOUND, "application/json", error_json.dump());
            write_and_close(response);
        }

    } catch (const std::exception &e) {
        spdlog::error("Error handling admin connection: {}", e.what());
    }
}

std::string AdminServer::parse_request(const std::string &request, std::string &path, std::string &body) {
    // Parse request line: METHOD SP PATH SP HTTP/VERSION
    auto line_end = request.find("\r\n");
    if (line_end == std::string::npos) {
        return "";
    }

    std::string_view request_line(request.data(), line_end);
    auto method_end = request_line.find(' ');
    if (method_end == std::string::npos || method_end == 0) {
        return "";
    }

    auto path_start = method_end + 1;
    auto path_end = request_line.find(' ', path_start);
    if (path_end == std::string::npos || path_end <= path_start) {
        return "";
    }

    std::string method(request_line.substr(0, method_end));
    path = std::string(request_line.substr(path_start, path_end - path_start));

    // Extract body (after double newline)
    size_t body_start = request.find("\r\n\r\n");
    if (body_start != std::string::npos) {
        body = request.substr(body_start + 4);
    }

    return method;
}

std::string AdminServer::build_response(int status_code, const std::string &content_type, const std::string &body) {
    std::string status_text;
    switch (status_code) {
    case HTTP_OK:
        status_text = "OK";
        break;
    case HTTP_BAD_REQUEST:
        status_text = "Bad Request";
        break;
    case HTTP_UNAUTHORIZED:
        status_text = "Unauthorized";
        break;
    case HTTP_NOT_FOUND:
        status_text = "Not Found";
        break;
    case HTTP_INTERNAL_ERROR:
        status_text = "Internal Server Error";
        break;
    default:
        status_text = "Unknown";
        break;
    }

    std::string cors_headers;
    if (!cors_origin_.empty()) {
        cors_headers = fmt::format(
            "Access-Control-Allow-Origin: {}\r\n"
            "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
            "Access-Control-Allow-Headers: Content-Type, Authorization\r\n",
            cors_origin_);
    }

    return fmt::format(
        "HTTP/1.1 {} {}\r\n"
        "Content-Type: {}\r\n"
        "Content-Length: {}\r\n"
        "{}"
        "\r\n"
        "{}",
        status_code, status_text, content_type, body.size(), cors_headers, body);
}

// Helper functions for zone reload

ReloadZoneRequest parse_reload_request(const std::string &json_str) {
    ReloadZoneRequest request;

    try {
        json j = json::parse(json_str);
        request.zone_id = j.value("zone_id", 0);
        request.force = j.value("force", false);
    } catch (const json::exception &e) {
        spdlog::error("Failed to parse reload request: {}", e.what());
        throw std::runtime_error(fmt::format("Invalid JSON: {}", e.what()));
    }

    return request;
}

std::string serialize_reload_response(const ReloadZoneResponse &response) {
    json j = {{"success", response.success},
              {"message", response.message},
              {"zones_reloaded", response.zones_reloaded},
              {"errors", response.errors}};

    return j.dump();
}

} // namespace fierymud

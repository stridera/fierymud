#include "admin_server.hpp"
#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cstdlib>
#include <iostream>

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
    constexpr size_t AUTH_BEARER_PREFIX_LEN = 22;  // Length of "Authorization: Bearer "
}

namespace fierymud {

AdminServer::AdminServer(uint16_t port, const std::string& bind_address)
    : port_(port), bind_address_(bind_address), running_(false) {
    // Load authentication token from environment
    const char* token_env = std::getenv("FIERYMUD_ADMIN_TOKEN");
    if (token_env) {
        auth_token_ = token_env;
    } else {
        // Generate a random token if none is set (not recommended for production)
        spdlog::warn("FIERYMUD_ADMIN_TOKEN not set, admin API will be unauthenticated");
        auth_token_ = "";
    }
}

AdminServer::~AdminServer() {
    stop();
}

void AdminServer::start() {
    if (running_.load()) {
        spdlog::warn("Admin server already running");
        return;
    }

    running_.store(true);
    io_context_ = std::make_unique<io_context>();

    // Create acceptor
    acceptor_ = std::make_unique<tcp::acceptor>(*io_context_,
        tcp::endpoint(asio::ip::make_address(bind_address_), port_));
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
        return;  // Already stopped
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

void AdminServer::register_handler(const std::string& path, CommandHandler handler) {
    handlers_[path] = std::move(handler);
    spdlog::debug("Registered admin handler for path: {}", path);
}

void AdminServer::start_accept() {
    if (!running_.load() || !acceptor_) {
        return;
    }

    auto socket = std::make_shared<tcp::socket>(*io_context_);

    acceptor_->async_accept(*socket, [this, socket](const std::error_code& ec) {
        if (ec) {
            if (ec != asio::error::operation_aborted && running_.load()) {
                spdlog::error("Admin server accept error: {}", ec.message());
            }
            return;  // Don't continue accepting on error
        }

        // Handle connection in the io_context thread (no need for separate thread)
        handle_connection(socket);

        // Accept next connection
        start_accept();
    });
}

void AdminServer::handle_connection(std::shared_ptr<tcp::socket> socket) {
    try {
        // Read HTTP request
        std::array<char, HTTP_REQUEST_BUFFER_SIZE> buffer;
        std::error_code ec;
        size_t bytes_read = socket->read_some(asio::buffer(buffer), ec);

        if (ec) {
            spdlog::error("Error reading from admin client: {}", ec.message());
            return;
        }

        std::string request(buffer.data(), bytes_read);
        std::string path, body;
        std::string method = parse_request(request, path, body);

        spdlog::debug("Admin request: {} {}", method, path);

        // Check authentication if token is set
        if (!auth_token_.empty()) {
            size_t auth_pos = request.find("Authorization: Bearer ");
            if (auth_pos == std::string::npos) {
                std::string response = build_response(HTTP_UNAUTHORIZED, "application/json",
                    R"({"error": "Unauthorized", "message": "Missing authentication token"})");
                asio::write(*socket, asio::buffer(response), ec);
                return;
            }

            size_t token_start = auth_pos + AUTH_BEARER_PREFIX_LEN;
            size_t token_end = request.find("\r\n", token_start);
            std::string provided_token = request.substr(token_start, token_end - token_start);

            if (provided_token != auth_token_) {
                std::string response = build_response(HTTP_UNAUTHORIZED, "application/json",
                    R"({"error": "Unauthorized", "message": "Invalid authentication token"})");
                asio::write(*socket, asio::buffer(response), ec);
                return;
            }
        }

        // Route to handler
        auto handler_it = handlers_.find(path);
        if (handler_it != handlers_.end()) {
            try {
                std::string response_body = handler_it->second(path, body);
                std::string response = build_response(HTTP_OK, "application/json", response_body);
                asio::write(*socket, asio::buffer(response), ec);
            } catch (const std::exception& e) {
                spdlog::error("Handler error for {}: {}", path, e.what());
                json error_json = {
                    {"error", "Internal Server Error"},
                    {"message", e.what()}
                };
                std::string response = build_response(HTTP_INTERNAL_ERROR, "application/json", error_json.dump());
                asio::write(*socket, asio::buffer(response), ec);
            }
        } else {
            json error_json = {
                {"error", "Not Found"},
                {"message", fmt::format("No handler registered for path: {}", path)}
            };
            std::string response = build_response(HTTP_NOT_FOUND, "application/json", error_json.dump());
            asio::write(*socket, asio::buffer(response), ec);
        }

        socket->shutdown(tcp::socket::shutdown_both, ec);
        socket->close(ec);

    } catch (const std::exception& e) {
        spdlog::error("Error handling admin connection: {}", e.what());
    }
}

std::string AdminServer::parse_request(const std::string& request, std::string& path, std::string& body) {
    // Simple HTTP parser - just extract method, path, and body
    size_t method_end = request.find(' ');
    std::string method = request.substr(0, method_end);

    size_t path_start = method_end + 1;
    size_t path_end = request.find(' ', path_start);
    path = request.substr(path_start, path_end - path_start);

    // Extract body (after double newline)
    size_t body_start = request.find("\r\n\r\n");
    if (body_start != std::string::npos) {
        body = request.substr(body_start + 4);
    }

    return method;
}

std::string AdminServer::build_response(int status_code, const std::string& content_type, const std::string& body) {
    std::string status_text;
    switch (status_code) {
        case HTTP_OK: status_text = "OK"; break;
        case HTTP_BAD_REQUEST: status_text = "Bad Request"; break;
        case HTTP_UNAUTHORIZED: status_text = "Unauthorized"; break;
        case HTTP_NOT_FOUND: status_text = "Not Found"; break;
        case HTTP_INTERNAL_ERROR: status_text = "Internal Server Error"; break;
        default: status_text = "Unknown"; break;
    }

    return fmt::format(
        "HTTP/1.1 {} {}\r\n"
        "Content-Type: {}\r\n"
        "Content-Length: {}\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type, Authorization\r\n"
        "\r\n"
        "{}",
        status_code, status_text, content_type, body.size(), body
    );
}

// Helper functions for zone reload

ReloadZoneRequest parse_reload_request(const std::string& json_str) {
    ReloadZoneRequest request;

    try {
        json j = json::parse(json_str);
        request.zone_id = j.value("zone_id", 0);
        request.force = j.value("force", false);
    } catch (const json::exception& e) {
        spdlog::error("Failed to parse reload request: {}", e.what());
        throw std::runtime_error(fmt::format("Invalid JSON: {}", e.what()));
    }

    return request;
}

std::string serialize_reload_response(const ReloadZoneResponse& response) {
    json j = {
        {"success", response.success},
        {"message", response.message},
        {"zones_reloaded", response.zones_reloaded},
        {"errors", response.errors}
    };

    return j.dump();
}

} // namespace fierymud

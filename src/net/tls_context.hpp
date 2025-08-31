/***************************************************************************
 *   File: src/net/tls_context.hpp             Part of FieryMUD *
 *  Usage: TLS/SSL context management for secure connections               *
 ***************************************************************************/

#pragma once

#include "../core/result.hpp"
#include <asio.hpp>
#include <asio/ssl.hpp>
#include <memory>
#include <string>

// Forward declarations
struct ServerConfig;

/**
 * @brief TLS Context Manager for SSL/TLS certificate and context management
 *
 * Manages SSL/TLS contexts for secure connections with proper certificate
 * loading, validation, and security settings.
 */
class TLSContextManager {
  public:
    explicit TLSContextManager(const ServerConfig& config);
    ~TLSContextManager() = default;

    // Initialize TLS context with certificates and security settings
    Result<void> initialize();
    
    // Get the SSL context for new connections
    asio::ssl::context& get_context() { return ssl_context_; }
    const asio::ssl::context& get_context() const { return ssl_context_; }
    
    // Certificate management
    Result<void> load_certificates();
    Result<void> load_dh_params();
    Result<void> configure_security_options();
    
    // Certificate validation
    bool verify_certificate(bool preverified, asio::ssl::verify_context& ctx);
    
    // Status queries
    bool is_initialized() const { return initialized_; }
    bool has_valid_certificates() const { return has_valid_certs_; }

  private:
    const ServerConfig& config_;
    asio::ssl::context ssl_context_;
    bool initialized_{false};
    bool has_valid_certs_{false};
    
    // Helper methods
    Result<void> create_self_signed_certificate();
    Result<void> setup_cipher_list();
    Result<void> setup_protocols();
    bool file_exists(const std::string& path) const;
};

/**
 * @brief TLS Socket wrapper for PlayerConnection
 *
 * Provides a unified interface for both plain and TLS sockets,
 * allowing PlayerConnection to work transparently with either.
 */
class TLSSocket {
  public:
    // Constructor for plain TCP socket
    explicit TLSSocket(asio::ip::tcp::socket socket);
    
    // Constructor for TLS socket
    TLSSocket(asio::ip::tcp::socket socket, asio::ssl::context& ssl_context);
    
    ~TLSSocket() = default;

    // Socket operations (unified interface)
    void async_handshake(std::function<void(const asio::error_code&)> handler);
    void async_read_some(asio::mutable_buffer buffer, 
                        std::function<void(const asio::error_code&, std::size_t)> handler);
    void async_write_some(asio::const_buffer buffer,
                         std::function<void(const asio::error_code&, std::size_t)> handler);
    void close();
    
    // Status queries
    bool is_tls() const { return is_tls_; }
    bool is_open() const;
    std::string remote_endpoint() const;
    
    // Access to underlying socket for accept operations
    asio::ip::tcp::socket& tcp_socket();
    const asio::ip::tcp::socket& tcp_socket() const;

  private:
    bool is_tls_;
    asio::ip::tcp::socket tcp_socket_;
    std::unique_ptr<asio::ssl::stream<asio::ip::tcp::socket&>> tls_stream_;
    bool handshake_complete_{false};
    
    void handle_handshake_complete();
};
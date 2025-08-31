/***************************************************************************
 *   File: src/net/tls_context.cpp             Part of FieryMUD *
 *  Usage: TLS/SSL context management implementation                       *
 ***************************************************************************/

#include "tls_context.hpp"
#include "../server/mud_server.hpp"
#include "../core/logging.hpp"
#include <filesystem>
#include <fstream>

// TLSContextManager implementation

TLSContextManager::TLSContextManager(const ServerConfig& config)
    : config_(config), ssl_context_(asio::ssl::context::tlsv12_server) {
    Log::debug("TLSContextManager created");
}

Result<void> TLSContextManager::initialize() {
    Log::info("Initializing TLS context...");
    
    try {
        // Configure basic SSL context options
        auto security_result = configure_security_options();
        if (!security_result) {
            return std::unexpected(security_result.error());
        }
        
        // Load certificates
        auto cert_result = load_certificates();
        if (!cert_result) {
            // Try to create self-signed certificates if loading fails
            Log::warn("Failed to load certificates, attempting to create self-signed");
            auto self_signed_result = create_self_signed_certificate();
            if (!self_signed_result) {
                return std::unexpected(self_signed_result.error());
            }
            // Retry loading after creating self-signed
            cert_result = load_certificates();
            if (!cert_result) {
                return std::unexpected(cert_result.error());
            }
        }
        
        // Load DH parameters if available
        auto dh_result = load_dh_params();
        if (!dh_result) {
            Log::warn("Failed to load DH parameters: {}", dh_result.error().message);
            Log::info("Continuing without custom DH parameters (will use defaults)");
        }
        
        initialized_ = true;
        Log::info("TLS context initialized successfully");
        return Success();
        
    } catch (const std::exception& e) {
        return std::unexpected(Error{ErrorCode::NetworkError, 
                                   fmt::format("TLS context initialization failed: {}", e.what())});
    }
}

Result<void> TLSContextManager::load_certificates() {
    try {
        if (!file_exists(config_.tls_certificate_file)) {
            return std::unexpected(Error{ErrorCode::FileNotFound,
                                       fmt::format("Certificate file not found: {}", config_.tls_certificate_file)});
        }
        
        if (!file_exists(config_.tls_private_key_file)) {
            return std::unexpected(Error{ErrorCode::FileNotFound,
                                       fmt::format("Private key file not found: {}", config_.tls_private_key_file)});
        }
        
        // Load certificate chain
        ssl_context_.use_certificate_chain_file(config_.tls_certificate_file);
        Log::debug("Loaded certificate chain from: {}", config_.tls_certificate_file);
        
        // Load private key
        ssl_context_.use_private_key_file(config_.tls_private_key_file, asio::ssl::context::pem);
        Log::debug("Loaded private key from: {}", config_.tls_private_key_file);
        
        has_valid_certs_ = true;
        return Success();
        
    } catch (const std::exception& e) {
        return std::unexpected(Error{ErrorCode::FileAccessError,
                                   fmt::format("Failed to load certificates: {}", e.what())});
    }
}

Result<void> TLSContextManager::load_dh_params() {
    try {
        if (!file_exists(config_.tls_dh_params_file)) {
            return std::unexpected(Error{ErrorCode::FileNotFound,
                                       fmt::format("DH params file not found: {}", config_.tls_dh_params_file)});
        }
        
        ssl_context_.use_tmp_dh_file(config_.tls_dh_params_file);
        Log::debug("Loaded DH parameters from: {}", config_.tls_dh_params_file);
        return Success();
        
    } catch (const std::exception& e) {
        return std::unexpected(Error{ErrorCode::FileAccessError,
                                   fmt::format("Failed to load DH parameters: {}", e.what())});
    }
}

Result<void> TLSContextManager::configure_security_options() {
    try {
        // Configure SSL context options for security
        ssl_context_.set_options(
            asio::ssl::context::default_workarounds |
            asio::ssl::context::no_sslv2 |
            asio::ssl::context::no_sslv3 |
            asio::ssl::context::no_tlsv1 |
            asio::ssl::context::no_tlsv1_1 |
            asio::ssl::context::single_dh_use
        );
        
        // Set up protocols and cipher list
        auto protocol_result = setup_protocols();
        if (!protocol_result) {
            return std::unexpected(protocol_result.error());
        }
        
        auto cipher_result = setup_cipher_list();
        if (!cipher_result) {
            return std::unexpected(cipher_result.error());
        }
        
        // Set up certificate verification if required
        if (config_.tls_require_client_cert) {
            ssl_context_.set_verify_mode(asio::ssl::verify_peer | asio::ssl::verify_fail_if_no_peer_cert);
            ssl_context_.set_verify_callback([this](bool preverified, asio::ssl::verify_context& ctx) {
                return verify_certificate(preverified, ctx);
            });
            Log::info("TLS client certificate verification enabled");
        } else {
            ssl_context_.set_verify_mode(asio::ssl::verify_none);
            Log::debug("TLS client certificate verification disabled");
        }
        
        return Success();
        
    } catch (const std::exception& e) {
        return std::unexpected(Error{ErrorCode::NetworkError,
                                   fmt::format("Failed to configure TLS security options: {}", e.what())});
    }
}

Result<void> TLSContextManager::setup_protocols() {
    // Protocols are set via context options in configure_security_options
    Log::debug("TLS protocols configured: TLSv1.2+ only");
    return Success();
}

Result<void> TLSContextManager::setup_cipher_list() {
    try {
        // Set secure cipher list (modern, secure ciphers only)
        SSL_CTX* ctx = ssl_context_.native_handle();
        const char* cipher_list = 
            "ECDHE-RSA-AES256-GCM-SHA384:"
            "ECDHE-RSA-AES128-GCM-SHA256:"
            "ECDHE-RSA-AES256-SHA384:"
            "ECDHE-RSA-AES128-SHA256:"
            "DHE-RSA-AES256-GCM-SHA384:"
            "DHE-RSA-AES128-GCM-SHA256";
            
        if (SSL_CTX_set_cipher_list(ctx, cipher_list) != 1) {
            return std::unexpected(Error{ErrorCode::NetworkError, "Failed to set TLS cipher list"});
        }
        
        Log::debug("TLS cipher list configured with secure ciphers");
        return Success();
        
    } catch (const std::exception& e) {
        return std::unexpected(Error{ErrorCode::NetworkError,
                                   fmt::format("Failed to setup cipher list: {}", e.what())});
    }
}

bool TLSContextManager::verify_certificate(bool preverified, asio::ssl::verify_context& ctx) {
    // Basic certificate verification - can be extended for more sophisticated validation
    char subject_name[256];
    X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
    X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
    
    Log::debug("TLS certificate verification: subject={}, preverified={}", subject_name, preverified);
    
    // For now, accept the certificate if it passed OpenSSL's built-in verification
    return preverified;
}

Result<void> TLSContextManager::create_self_signed_certificate() {
    Log::info("Creating self-signed certificate for development...");
    
    // Create certs directory if it doesn't exist
    std::filesystem::path cert_dir = std::filesystem::path(config_.tls_certificate_file).parent_path();
    try {
        std::filesystem::create_directories(cert_dir);
    } catch (const std::exception& e) {
        return std::unexpected(Error{ErrorCode::FileAccessError,
                                   fmt::format("Failed to create certificate directory: {}", e.what())});
    }
    
    // For now, just log that we would create a self-signed certificate
    // Full implementation would use OpenSSL to generate a certificate
    Log::warn("Self-signed certificate generation not yet implemented");
    Log::info("Please create TLS certificates manually:");
    Log::info("  Certificate: {}", config_.tls_certificate_file);
    Log::info("  Private key: {}", config_.tls_private_key_file);
    Log::info("  DH params:   {}", config_.tls_dh_params_file);
    
    return std::unexpected(Error{ErrorCode::NotImplemented, "Self-signed certificate generation not implemented"});
}

bool TLSContextManager::file_exists(const std::string& path) const {
    return std::filesystem::exists(path);
}

// TLSSocket implementation

TLSSocket::TLSSocket(asio::ip::tcp::socket socket)
    : is_tls_(false), tcp_socket_(std::move(socket)) {
    Log::debug("Created plain TCP socket wrapper");
}

TLSSocket::TLSSocket(asio::ip::tcp::socket socket, asio::ssl::context& ssl_context)
    : is_tls_(true), tcp_socket_(std::move(socket)) {
    tls_stream_ = std::make_unique<asio::ssl::stream<asio::ip::tcp::socket&>>(tcp_socket_, ssl_context);
    Log::debug("Created TLS socket wrapper");
}

void TLSSocket::async_handshake(std::function<void(const asio::error_code&)> handler) {
    if (!is_tls_) {
        // Plain socket - no handshake needed
        asio::post(tcp_socket_.get_executor(), [handler]() {
            handler(asio::error_code{});
        });
        return;
    }
    
    if (!tls_stream_) {
        asio::post(tcp_socket_.get_executor(), [handler]() {
            handler(asio::error::invalid_argument);
        });
        return;
    }
    
    tls_stream_->async_handshake(asio::ssl::stream_base::server, [this, handler](const asio::error_code& error) {
        if (!error) {
            handshake_complete_ = true;
            Log::debug("TLS handshake completed successfully");
        } else {
            Log::warn("TLS handshake failed: {}", error.message());
        }
        handler(error);
    });
}

void TLSSocket::async_read_some(asio::mutable_buffer buffer,
                               std::function<void(const asio::error_code&, std::size_t)> handler) {
    if (is_tls_) {
        if (!tls_stream_ || !handshake_complete_) {
            asio::post(tcp_socket_.get_executor(), [handler]() {
                handler(asio::error::not_connected, 0);
            });
            return;
        }
        tls_stream_->async_read_some(buffer, handler);
    } else {
        tcp_socket_.async_read_some(buffer, handler);
    }
}

void TLSSocket::async_write_some(asio::const_buffer buffer,
                                std::function<void(const asio::error_code&, std::size_t)> handler) {
    if (is_tls_) {
        if (!tls_stream_ || !handshake_complete_) {
            asio::post(tcp_socket_.get_executor(), [handler]() {
                handler(asio::error::not_connected, 0);
            });
            return;
        }
        tls_stream_->async_write_some(buffer, handler);
    } else {
        tcp_socket_.async_write_some(buffer, handler);
    }
}

void TLSSocket::close() {
    try {
        if (is_tls_ && tls_stream_ && handshake_complete_) {
            // Graceful TLS shutdown
            tls_stream_->async_shutdown([](const asio::error_code&) {
                // Shutdown complete or failed - either way, continue with close
            });
        }
        if (tcp_socket_.is_open()) {
            tcp_socket_.close();
        }
    } catch (const std::exception& e) {
        Log::warn("Error during TLS socket close: {}", e.what());
    }
}

bool TLSSocket::is_open() const {
    return tcp_socket_.is_open();
}

std::string TLSSocket::remote_endpoint() const {
    try {
        return tcp_socket_.remote_endpoint().address().to_string();
    } catch (const std::exception&) {
        return "unknown";
    }
}

asio::ip::tcp::socket& TLSSocket::tcp_socket() {
    return tcp_socket_;
}

const asio::ip::tcp::socket& TLSSocket::tcp_socket() const {
    return tcp_socket_;
}
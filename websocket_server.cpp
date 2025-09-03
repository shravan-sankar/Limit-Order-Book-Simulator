#include "websocket_server.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>

WebSocketServer::WebSocketServer() : m_running(false) {
    // Initialize the server
    m_server.init_asio();
    
    // Set up event handlers
    m_server.set_open_handler([this](connection_hdl hdl) {
        this->on_open(hdl);
    });
    
    m_server.set_close_handler([this](connection_hdl hdl) {
        this->on_close(hdl);
    });
    
    m_server.set_message_handler([this](connection_hdl hdl, message_ptr msg) {
        this->on_message(hdl, msg);
    });
    
    m_server.set_http_handler([this](connection_hdl hdl) {
        this->on_http(hdl);
    });
}

WebSocketServer::~WebSocketServer() {
    stop();
}

bool WebSocketServer::start(int port) {
    try {
        m_server.listen(port);
        m_server.start_accept();
        m_running = true;
        
        std::cout << "WebSocket server started on port " << port << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to start WebSocket server: " << e.what() << std::endl;
        return false;
    }
}

void WebSocketServer::stop() {
    if (m_running) {
        m_running = false;
        m_server.stop();
        if (m_server_thread.joinable()) {
            m_server_thread.join();
        }
        std::cout << "WebSocket server stopped" << std::endl;
    }
}

void WebSocketServer::run() {
    if (m_running) {
        m_server_thread = std::thread([this]() {
            try {
                m_server.run();
            } catch (const std::exception& e) {
                std::cerr << "WebSocket server error: " << e.what() << std::endl;
            }
        });
    }
}

void WebSocketServer::on_open(connection_hdl hdl) {
    std::lock_guard<std::mutex> lock(m_connection_mutex);
    m_connections.insert(hdl);
    std::cout << "Client connected. Total connections: " << m_connections.size() << std::endl;
    
    // Send welcome message
    json welcome = {
        {"type", "welcome"},
        {"message", "Connected to Limit Order Book Trading System"},
        {"timestamp", std::time(nullptr)}
    };
    send_message(hdl, welcome);
}

void WebSocketServer::on_close(connection_hdl hdl) {
    std::lock_guard<std::mutex> lock(m_connection_mutex);
    m_connections.erase(hdl);
    std::cout << "Client disconnected. Total connections: " << m_connections.size() << std::endl;
}

void WebSocketServer::on_message(connection_hdl hdl, message_ptr msg) {
    try {
        std::string message = msg->get_payload();
        process_message(message, hdl);
    } catch (const std::exception& e) {
        std::cerr << "Error processing message: " << e.what() << std::endl;
        send_error(hdl, "Error processing message: " + std::string(e.what()));
    }
}

void WebSocketServer::on_http(connection_hdl hdl) {
    // Handle HTTP requests (for CORS, etc.)
    server::connection_ptr con = m_server.get_con_from_hdl(hdl);
    
    // Set CORS headers
    con->append_header("Access-Control-Allow-Origin", "*");
    con->append_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    con->append_header("Access-Control-Allow-Headers", "Content-Type");
    
    if (con->get_resource() == "/") {
        con->set_body("WebSocket server is running");
        con->set_status(websocketpp::http::status_code::ok);
    } else {
        con->set_status(websocketpp::http::status_code::not_found);
    }
}

void WebSocketServer::process_message(const std::string& message, connection_hdl hdl) {
    try {
        json data = json::parse(message);
        std::string type = data.value("type", "");
        
        if (type == "submit_order") {
            handle_order_submission(data, hdl);
        } else if (type == "cancel_order") {
            std::string orderId = data.value("orderId", "");
            handle_order_cancellation(orderId, hdl);
        } else if (type == "ping") {
            json pong = {{"type", "pong"}, {"timestamp", std::time(nullptr)}};
            send_message(hdl, pong);
        } else {
            send_error(hdl, "Unknown message type: " + type);
        }
    } catch (const json::exception& e) {
        send_error(hdl, "Invalid JSON: " + std::string(e.what()));
    }
}

void WebSocketServer::handle_order_submission(const json& orderData, connection_hdl hdl) {
    try {
        if (!m_submit_callback) {
            send_error(hdl, "Matching engine not connected");
            return;
        }
        
        OrderType type = string_to_order_type(orderData.value("orderType", ""));
        double price = orderData.value("price", 0.0);
        int quantity = orderData.value("quantity", 0);
        std::string symbol = orderData.value("symbol", "DEFAULT");
        std::string clientId = orderData.value("clientId", "WEB_CLIENT");
        
        if (price <= 0 || quantity <= 0) {
            send_error(hdl, "Invalid price or quantity");
            return;
        }
        
        std::string orderId = m_submit_callback(type, price, quantity, symbol, clientId);
        
        if (orderId.empty()) {
            send_error(hdl, "Failed to submit order");
            return;
        }
        
        json response = {
            {"type", "order_submitted"},
            {"orderId", orderId},
            {"status", "success"},
            {"timestamp", std::time(nullptr)}
        };
        send_message(hdl, response);
        
    } catch (const std::exception& e) {
        send_error(hdl, "Error submitting order: " + std::string(e.what()));
    }
}

void WebSocketServer::handle_order_cancellation(const std::string& orderId, connection_hdl hdl) {
    try {
        if (!m_cancel_callback) {
            send_error(hdl, "Matching engine not connected");
            return;
        }
        
        bool success = m_cancel_callback(orderId);
        
        json response = {
            {"type", "order_cancelled"},
            {"orderId", orderId},
            {"status", success ? "success" : "failed"},
            {"timestamp", std::time(nullptr)}
        };
        send_message(hdl, response);
        
    } catch (const std::exception& e) {
        send_error(hdl, "Error cancelling order: " + std::string(e.what()));
    }
}

void WebSocketServer::broadcast_trade(const Trade& trade) {
    json tradeData = {
        {"type", "trade"},
        {"tradeId", trade.tradeId},
        {"symbol", trade.symbol},
        {"price", trade.price},
        {"quantity", trade.quantity},
        {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(
            trade.timestamp.time_since_epoch()).count()}
    };
    
    std::lock_guard<std::mutex> lock(m_connection_mutex);
    for (auto& hdl : m_connections) {
        try {
            send_message(hdl, tradeData);
        } catch (const std::exception& e) {
            std::cerr << "Error broadcasting trade: " << e.what() << std::endl;
        }
    }
}

void WebSocketServer::broadcast_orderbook_update(const std::string& symbol, double bestBid, double bestAsk, int bidSize, int askSize) {
    json orderbookData = {
        {"type", "orderbook_update"},
        {"symbol", symbol},
        {"bestBid", bestBid},
        {"bestAsk", bestAsk},
        {"bidSize", bidSize},
        {"askSize", askSize},
        {"spread", bestAsk - bestBid},
        {"timestamp", std::time(nullptr)}
    };
    
    std::lock_guard<std::mutex> lock(m_connection_mutex);
    for (auto& hdl : m_connections) {
        try {
            send_message(hdl, orderbookData);
        } catch (const std::exception& e) {
            std::cerr << "Error broadcasting orderbook update: " << e.what() << std::endl;
        }
    }
}

void WebSocketServer::broadcast_order_status(const std::string& orderId, const std::string& status, const std::string& message) {
    json statusData = {
        {"type", "order_status"},
        {"orderId", orderId},
        {"status", status},
        {"message", message},
        {"timestamp", std::time(nullptr)}
    };
    
    std::lock_guard<std::mutex> lock(m_connection_mutex);
    for (auto& hdl : m_connections) {
        try {
            send_message(hdl, statusData);
        } catch (const std::exception& e) {
            std::cerr << "Error broadcasting order status: " << e.what() << std::endl;
        }
    }
}

void WebSocketServer::set_matching_engine_callback(std::function<std::string(OrderType, double, int, const std::string&, const std::string&)> submit_callback) {
    m_submit_callback = submit_callback;
}

void WebSocketServer::set_cancel_callback(std::function<bool(const std::string&)> cancel_callback) {
    m_cancel_callback = cancel_callback;
}

void WebSocketServer::send_message(connection_hdl hdl, const json& message) {
    try {
        server::connection_ptr con = m_server.get_con_from_hdl(hdl);
        con->send(message.dump(), websocketpp::frame::opcode::text);
    } catch (const std::exception& e) {
        std::cerr << "Error sending message: " << e.what() << std::endl;
    }
}

void WebSocketServer::send_error(connection_hdl hdl, const std::string& error) {
    json errorData = {
        {"type", "error"},
        {"message", error},
        {"timestamp", std::time(nullptr)}
    };
    send_message(hdl, errorData);
}

OrderType WebSocketServer::string_to_order_type(const std::string& type) {
    if (type == "BUY") return BUY;
    if (type == "SELL") return SELL;
    return BUY; // Default
}

std::string WebSocketServer::order_type_to_string(OrderType type) {
    return (type == BUY) ? "BUY" : "SELL";
}

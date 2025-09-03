#ifndef WEBSOCKET_SERVER_HPP
#define WEBSOCKET_SERVER_HPP

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <nlohmann/json.hpp>
#include <thread>
#include <mutex>
#include <set>
#include <functional>
#include "order.hpp"

using json = nlohmann::json;

class WebSocketServer {
public:
    using server = websocketpp::server<websocketpp::config::asio>;
    using connection_hdl = websocketpp::connection_hdl;
    using message_ptr = websocketpp::config::asio::message_type::ptr;

    WebSocketServer();
    ~WebSocketServer();

    // Server management
    bool start(int port = 8080);
    void stop();
    void run();

    // Message broadcasting
    void broadcast_trade(const Trade& trade);
    void broadcast_orderbook_update(const std::string& symbol, double bestBid, double bestAsk, int bidSize, int askSize);
    void broadcast_order_status(const std::string& orderId, const std::string& status, const std::string& message = "");

    // Order handling
    void handle_order_submission(const json& orderData, connection_hdl hdl);
    void handle_order_cancellation(const std::string& orderId, connection_hdl hdl);

    // Set matching engine callback
    void set_matching_engine_callback(std::function<std::string(OrderType, double, int, const std::string&, const std::string&)> submit_callback);
    void set_cancel_callback(std::function<bool(const std::string&)> cancel_callback);

private:
    server m_server;
    std::set<connection_hdl, std::owner_less<connection_hdl>> m_connections;
    std::mutex m_connection_mutex;
    std::thread m_server_thread;
    bool m_running;

    // Matching engine callbacks
    std::function<std::string(OrderType, double, int, const std::string&, const std::string&)> m_submit_callback;
    std::function<bool(const std::string&)> m_cancel_callback;

    // WebSocket event handlers
    void on_open(connection_hdl hdl);
    void on_close(connection_hdl hdl);
    void on_message(connection_hdl hdl, message_ptr msg);
    void on_http(connection_hdl hdl);

    // Message processing
    void process_message(const std::string& message, connection_hdl hdl);
    void send_message(connection_hdl hdl, const json& message);
    void send_error(connection_hdl hdl, const std::string& error);

    // Helper functions
    OrderType string_to_order_type(const std::string& type);
    std::string order_type_to_string(OrderType type);
};

#endif // WEBSOCKET_SERVER_HPP

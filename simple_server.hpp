#ifndef SIMPLE_SERVER_HPP
#define SIMPLE_SERVER_HPP

#include <iostream>
#include <thread>
#include <mutex>
#include <set>
#include <functional>
#include <string>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include "order.hpp"

class SimpleServer {
public:
    SimpleServer();
    ~SimpleServer();

    // Server management
    bool start(int port = 8080);
    void stop();
    void run();

    // Message broadcasting
    void broadcast_trade(const Trade& trade);
    void broadcast_orderbook_update(const std::string& symbol, double bestBid, double bestAsk, int bidSize, int askSize);
    void broadcast_order_status(const std::string& orderId, const std::string& status, const std::string& message = "");

    // Order handling
    void handle_order_submission(const std::string& orderData, int clientSocket);
    void handle_order_cancellation(const std::string& orderId, int clientSocket);

    // Set matching engine callback
    void set_matching_engine_callback(std::function<std::string(OrderType, double, int, const std::string&, const std::string&)> submit_callback);
    void set_cancel_callback(std::function<bool(const std::string&)> cancel_callback);

private:
    int serverSocket;
    std::set<int> clientSockets;
    std::mutex clientMutex;
    std::thread serverThread;
    bool running;

    // Matching engine callbacks
    std::function<std::string(OrderType, double, int, const std::string&, const std::string&)> submitCallback;
    std::function<bool(const std::string&)> cancelCallback;

    // Server functions
    void serverWorker();
    void handleClient(int clientSocket);
    void sendMessage(int clientSocket, const std::string& message);
    void broadcastMessage(const std::string& message);
    
    // Helper functions
    OrderType string_to_order_type(const std::string& type);
    std::string order_type_to_string(OrderType type);
    std::string createJsonResponse(const std::string& type, const std::string& data);
};

#endif // SIMPLE_SERVER_HPP



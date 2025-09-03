#include "simple_server.hpp"
#include <cstring>
#include <algorithm>

SimpleServer::SimpleServer() : serverSocket(-1), running(false) {
}

SimpleServer::~SimpleServer() {
    stop();
}

bool SimpleServer::start(int port) {
    // Create socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        return false;
    }

    // Set socket options
    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Failed to set socket options" << std::endl;
        close(serverSocket);
        return false;
    }

    // Bind socket
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(serverSocket, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Failed to bind socket to port " << port << std::endl;
        close(serverSocket);
        return false;
    }

    // Listen for connections
    if (listen(serverSocket, 5) < 0) {
        std::cerr << "Failed to listen on socket" << std::endl;
        close(serverSocket);
        return false;
    }

    running = true;
    std::cout << "Simple server started on port " << port << std::endl;
    return true;
}

void SimpleServer::stop() {
    if (running) {
        running = false;
        
        // Close all client connections
        std::lock_guard<std::mutex> lock(clientMutex);
        for (int clientSocket : clientSockets) {
            close(clientSocket);
        }
        clientSockets.clear();
        
        // Close server socket
        if (serverSocket >= 0) {
            close(serverSocket);
            serverSocket = -1;
        }
        
        if (serverThread.joinable()) {
            serverThread.join();
        }
        
        std::cout << "Simple server stopped" << std::endl;
    }
}

void SimpleServer::run() {
    if (running) {
        serverThread = std::thread([this]() {
            this->serverWorker();
        });
    }
}

void SimpleServer::serverWorker() {
    while (running) {
        struct sockaddr_in clientAddress;
        socklen_t clientAddressLength = sizeof(clientAddress);
        
        int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddress, &clientAddressLength);
        
        if (clientSocket < 0) {
            if (running) {
                std::cerr << "Failed to accept client connection" << std::endl;
            }
            continue;
        }
        
        // Add client to set
        {
            std::lock_guard<std::mutex> lock(clientMutex);
            clientSockets.insert(clientSocket);
        }
        
        std::cout << "Client connected. Total connections: " << clientSockets.size() << std::endl;
        
        // Handle client in separate thread
        std::thread clientThread([this, clientSocket]() {
            this->handleClient(clientSocket);
        });
        clientThread.detach();
    }
}

void SimpleServer::handleClient(int clientSocket) {
    char buffer[1024];
    
    // Send welcome message
    std::string welcome = createJsonResponse("welcome", "Connected to Limit Order Book Trading System");
    sendMessage(clientSocket, welcome);
    
    while (running) {
        memset(buffer, 0, sizeof(buffer));
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        
        if (bytesReceived <= 0) {
            break; // Client disconnected
        }
        
        std::string message(buffer);
        
        // Simple message parsing (in a real implementation, you'd use a proper JSON parser)
        if (message.find("submit_order") != std::string::npos) {
            handle_order_submission(message, clientSocket);
        } else if (message.find("cancel_order") != std::string::npos) {
            // Extract order ID from message
            size_t start = message.find("orderId");
            if (start != std::string::npos) {
                start = message.find(":", start) + 1;
                size_t end = message.find(",", start);
                if (end == std::string::npos) end = message.find("}", start);
                std::string orderId = message.substr(start, end - start);
                // Remove quotes
                orderId.erase(std::remove(orderId.begin(), orderId.end(), '"'), orderId.end());
                handle_order_cancellation(orderId, clientSocket);
            }
        }
    }
    
    // Remove client from set
    {
        std::lock_guard<std::mutex> lock(clientMutex);
        clientSockets.erase(clientSocket);
    }
    
    close(clientSocket);
    std::cout << "Client disconnected. Total connections: " << clientSockets.size() << std::endl;
}

void SimpleServer::handle_order_submission(const std::string& orderData, int clientSocket) {
    try {
        if (!submitCallback) {
            sendMessage(clientSocket, createJsonResponse("error", "Matching engine not connected"));
            return;
        }
        
        // Simple parsing (in production, use proper JSON parser)
        // Extract values from JSON-like string
        double price = 0.0;
        int quantity = 0;
        std::string orderType = "BUY";
        std::string symbol = "DEFAULT";
        std::string clientId = "WEB_CLIENT";
        
        // Find price
        size_t pos = orderData.find("\"price\":");
        if (pos != std::string::npos) {
            pos += 8;
            size_t end = orderData.find(",", pos);
            if (end == std::string::npos) end = orderData.find("}", pos);
            price = std::stod(orderData.substr(pos, end - pos));
        }
        
        // Find quantity
        pos = orderData.find("\"quantity\":");
        if (pos != std::string::npos) {
            pos += 11;
            size_t end = orderData.find(",", pos);
            if (end == std::string::npos) end = orderData.find("}", pos);
            quantity = std::stoi(orderData.substr(pos, end - pos));
        }
        
        // Find order type
        pos = orderData.find("\"orderType\":");
        if (pos != std::string::npos) {
            pos += 12;
            size_t end = orderData.find(",", pos);
            if (end == std::string::npos) end = orderData.find("}", pos);
            orderType = orderData.substr(pos, end - pos);
            orderType.erase(std::remove(orderType.begin(), orderType.end(), '"'), orderType.end());
        }
        
        if (price <= 0 || quantity <= 0) {
            sendMessage(clientSocket, createJsonResponse("error", "Invalid price or quantity"));
            return;
        }
        
        OrderType type = string_to_order_type(orderType);
        std::string orderId = submitCallback(type, price, quantity, symbol, clientId);
        
        if (orderId.empty()) {
            sendMessage(clientSocket, createJsonResponse("error", "Failed to submit order"));
            return;
        }
        
        std::string response = "{\"type\":\"order_submitted\",\"orderId\":\"" + orderId + "\",\"status\":\"success\"}";
        sendMessage(clientSocket, response);
        
    } catch (const std::exception& e) {
        sendMessage(clientSocket, createJsonResponse("error", "Error submitting order: " + std::string(e.what())));
    }
}

void SimpleServer::handle_order_cancellation(const std::string& orderId, int clientSocket) {
    try {
        if (!cancelCallback) {
            sendMessage(clientSocket, createJsonResponse("error", "Matching engine not connected"));
            return;
        }
        
        bool success = cancelCallback(orderId);
        
        std::string response = "{\"type\":\"order_cancelled\",\"orderId\":\"" + orderId + "\",\"status\":\"" + (success ? "success" : "failed") + "\"}";
        sendMessage(clientSocket, response);
        
    } catch (const std::exception& e) {
        sendMessage(clientSocket, createJsonResponse("error", "Error cancelling order: " + std::string(e.what())));
    }
}

void SimpleServer::broadcast_trade(const Trade& trade) {
    std::string tradeData = "{\"type\":\"trade\",\"tradeId\":\"" + trade.tradeId + 
                           "\",\"symbol\":\"" + trade.symbol + 
                           "\",\"price\":" + std::to_string(trade.price) + 
                           ",\"quantity\":" + std::to_string(trade.quantity) + "}";
    
    broadcastMessage(tradeData);
}

void SimpleServer::broadcast_orderbook_update(const std::string& symbol, double bestBid, double bestAsk, int bidSize, int askSize) {
    std::string orderbookData = "{\"type\":\"orderbook_update\",\"symbol\":\"" + symbol + 
                               "\",\"bestBid\":" + std::to_string(bestBid) + 
                               ",\"bestAsk\":" + std::to_string(bestAsk) + 
                               ",\"bidSize\":" + std::to_string(bidSize) + 
                               ",\"askSize\":" + std::to_string(askSize) + 
                               ",\"spread\":" + std::to_string(bestAsk - bestBid) + "}";
    
    broadcastMessage(orderbookData);
}

void SimpleServer::broadcast_order_status(const std::string& orderId, const std::string& status, const std::string& message) {
    std::string statusData = "{\"type\":\"order_status\",\"orderId\":\"" + orderId + 
                            "\",\"status\":\"" + status + 
                            "\",\"message\":\"" + message + "\"}";
    
    broadcastMessage(statusData);
}

void SimpleServer::set_matching_engine_callback(std::function<std::string(OrderType, double, int, const std::string&, const std::string&)> submit_callback) {
    submitCallback = submit_callback;
}

void SimpleServer::set_cancel_callback(std::function<bool(const std::string&)> cancel_callback) {
    cancelCallback = cancel_callback;
}

void SimpleServer::sendMessage(int clientSocket, const std::string& message) {
    std::string fullMessage = message + "\n";
    send(clientSocket, fullMessage.c_str(), fullMessage.length(), 0);
}

void SimpleServer::broadcastMessage(const std::string& message) {
    std::lock_guard<std::mutex> lock(clientMutex);
    for (int clientSocket : clientSockets) {
        try {
            sendMessage(clientSocket, message);
        } catch (const std::exception& e) {
            std::cerr << "Error broadcasting message: " << e.what() << std::endl;
        }
    }
}

std::string SimpleServer::createJsonResponse(const std::string& type, const std::string& data) {
    return "{\"type\":\"" + type + "\",\"message\":\"" + data + "\"}";
}

OrderType SimpleServer::string_to_order_type(const std::string& type) {
    if (type == "BUY") return BUY;
    if (type == "SELL") return SELL;
    return BUY; // Default
}

std::string SimpleServer::order_type_to_string(OrderType type) {
    return (type == BUY) ? "BUY" : "SELL";
}



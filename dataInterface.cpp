// dataInterface.cpp
#include "dataInterface.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <random>
#include <chrono>
#include <algorithm>
#include <iomanip>
#include <ctime>

DataInterface::DataInterface(MatchingEngine& engine) 
    : matchingEngine(engine), simulationRunning(false) {
    // Set up trade callback
    matchingEngine.set_trade_callback([this](const Trade& trade) {
        on_trade_executed(trade);
    });
}

DataInterface::~DataInterface() {
    stop_simulation();
}

bool DataInterface::load_orders_from_csv(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return false;
    }
    
    std::string line;
    std::vector<std::shared_ptr<Order>> orders;
    
    // Skip header if present
    if (std::getline(file, line)) {
        if (line.find("type") != std::string::npos || line.find("Type") != std::string::npos) {
            // Header line, continue
        } else {
            // First data line, parse it
            auto order = parse_csv_line(line);
            if (order) orders.push_back(order);
        }
    }
    
    while (std::getline(file, line)) {
        auto order = parse_csv_line(line);
        if (order) orders.push_back(order);
    }
    
    file.close();
    
    // Process all orders
    matchingEngine.process_orders_batch(orders);
    
    std::cout << "Loaded " << orders.size() << " orders from " << filename << std::endl;
    return true;
}

bool DataInterface::load_orders_from_json(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return false;
    }
    
    std::string line;
    std::vector<std::shared_ptr<Order>> orders;
    
    while (std::getline(file, line)) {
        auto order = parse_json_line(line);
        if (order) orders.push_back(order);
    }
    
    file.close();
    
    // Process all orders
    matchingEngine.process_orders_batch(orders);
    
    std::cout << "Loaded " << orders.size() << " orders from " << filename << std::endl;
    return true;
}

void DataInterface::start_market_data_simulation(const std::string& symbol, double basePrice, int numOrders) {
    if (simulationRunning) {
        std::cout << "Simulation already running" << std::endl;
        return;
    }
    
    simulationRunning = true;
    simulationThread = std::thread(&DataInterface::simulation_worker, this, symbol, basePrice, numOrders);
    
    std::cout << "Started market data simulation for " << symbol 
              << " with base price " << basePrice << std::endl;
}

void DataInterface::stop_simulation() {
    if (simulationRunning) {
        simulationRunning = false;
        if (simulationThread.joinable()) {
            simulationThread.join();
        }
        std::cout << "Market data simulation stopped" << std::endl;
    }
}

void DataInterface::add_manual_order(OrderType type, double price, int quantity, const std::string& symbol) {
    std::string orderId = matchingEngine.submit_order(type, price, quantity, symbol);
    if (!orderId.empty()) {
        std::cout << "Manual order submitted: " << orderId << std::endl;
    } else {
        std::cout << "Failed to submit manual order" << std::endl;
    }
}

void DataInterface::print_statistics() const {
    std::lock_guard<std::mutex> lock(statsMutex);
    
    std::cout << "\n=== TRADING STATISTICS ===" << std::endl;
    std::cout << "Total Trades: " << totalTrades << std::endl;
    std::cout << "Total Volume: " << std::fixed << std::setprecision(2) << totalVolume << std::endl;
    std::cout << "Average Trade Size: " << (totalTrades > 0 ? totalVolume / totalTrades : 0.0) << std::endl;
    
    if (!tradeHistory.empty()) {
        auto latestTrade = tradeHistory.back();
        std::cout << "Latest Trade: " << latestTrade.quantity << " @ " 
                  << std::fixed << std::setprecision(2) << latestTrade.price << std::endl;
    }
    
    std::cout << "Best Bid: " << std::fixed << std::setprecision(2) << matchingEngine.get_best_bid() << std::endl;
    std::cout << "Best Ask: " << std::fixed << std::setprecision(2) << matchingEngine.get_best_ask() << std::endl;
    std::cout << "Spread: " << std::fixed << std::setprecision(2) << matchingEngine.get_spread() << std::endl;
    std::cout << "========================\n" << std::endl;
}

void DataInterface::set_trade_callback(std::function<void(const Trade&)> callback) {
    matchingEngine.set_trade_callback(callback);
}

void DataInterface::process_orders_from_file(const std::string& filename) {
    // Determine file type and process accordingly
    if (filename.substr(filename.find_last_of(".") + 1) == "csv") {
        load_orders_from_csv(filename);
    } else if (filename.substr(filename.find_last_of(".") + 1) == "json") {
        load_orders_from_json(filename);
    } else {
        std::cerr << "Unsupported file format. Use .csv or .json files." << std::endl;
    }
}

void DataInterface::simulation_worker(const std::string& symbol, double basePrice, int numOrders) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> priceDist(0.95, 1.05); // ±5% price variation
    std::uniform_int_distribution<> qtyDist(1, 100);
    std::uniform_int_distribution<> typeDist(0, 1);
    
    for (int i = 0; i < numOrders && simulationRunning; ++i) {
        // Generate random order
        OrderType type = (typeDist(gen) == 0) ? BUY : SELL;
        double price = basePrice * priceDist(gen);
        int quantity = qtyDist(gen);
        
        // Round price to 2 decimal places
        price = std::round(price * 100.0) / 100.0;
        
        // Submit order
        std::string orderId = matchingEngine.submit_order(type, price, quantity, symbol);
        
        // Small delay to simulate real-time data
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    simulationRunning = false;
}

void DataInterface::process_order_queue() {
    while (simulationRunning) {
        std::unique_lock<std::mutex> lock(queueMutex);
        queueCondition.wait(lock, [this] { return !orderQueue.empty() || !simulationRunning; });
        
        while (!orderQueue.empty()) {
            auto order = orderQueue.front();
            orderQueue.pop();
            lock.unlock();
            
            // Process order through matching engine
            // This would be implemented based on your specific needs
            
            lock.lock();
        }
    }
}

std::shared_ptr<Order> DataInterface::parse_csv_line(const std::string& line) {
    std::istringstream iss(line);
    std::string token;
    std::vector<std::string> tokens;
    
    while (std::getline(iss, token, ',')) {
        tokens.push_back(token);
    }
    
    if (tokens.size() < 3) return nullptr;
    
    try {
        OrderType type = (tokens[0] == "BUY" || tokens[0] == "buy") ? BUY : SELL;
        double price = std::stod(tokens[1]);
        int quantity = std::stoi(tokens[2]);
        std::string symbol = (tokens.size() > 3) ? tokens[3] : "DEFAULT";
        std::string clientId = (tokens.size() > 4) ? tokens[4] : "CSV_CLIENT";
        
        return std::make_shared<Order>("CSV_" + std::to_string(std::time(nullptr)), 
                                     type, price, quantity, symbol, clientId);
    } catch (const std::exception& e) {
        std::cerr << "Error parsing CSV line: " << line << " - " << e.what() << std::endl;
        return nullptr;
    }
}

std::shared_ptr<Order> DataInterface::parse_json_line(const std::string& line) {
    // Simple JSON parsing - in production, use a proper JSON library
    // Expected format: {"type":"BUY","price":100.0,"quantity":10,"symbol":"AAPL"}
    
    if (line.find("\"type\"") == std::string::npos) return nullptr;
    
    try {
        OrderType type = BUY;
        double price = 0.0;
        int quantity = 0;
        std::string symbol = "DEFAULT";
        
        // Extract type
        size_t typePos = line.find("\"type\"");
        if (typePos != std::string::npos) {
            size_t valueStart = line.find("\"", typePos + 7) + 1;
            size_t valueEnd = line.find("\"", valueStart);
            std::string typeStr = line.substr(valueStart, valueEnd - valueStart);
            type = (typeStr == "BUY") ? BUY : SELL;
        }
        
        // Extract price
        size_t pricePos = line.find("\"price\"");
        if (pricePos != std::string::npos) {
            size_t valueStart = line.find(":", pricePos) + 1;
            size_t valueEnd = line.find_first_of(",}", valueStart);
            price = std::stod(line.substr(valueStart, valueEnd - valueStart));
        }
        
        // Extract quantity
        size_t qtyPos = line.find("\"quantity\"");
        if (qtyPos != std::string::npos) {
            size_t valueStart = line.find(":", qtyPos) + 1;
            size_t valueEnd = line.find_first_of(",}", valueStart);
            quantity = std::stoi(line.substr(valueStart, valueEnd - valueStart));
        }
        
        // Extract symbol
        size_t symPos = line.find("\"symbol\"");
        if (symPos != std::string::npos) {
            size_t valueStart = line.find("\"", symPos + 9) + 1;
            size_t valueEnd = line.find("\"", valueStart);
            symbol = line.substr(valueStart, valueEnd - valueStart);
        }
        
        return std::make_shared<Order>("JSON_" + std::to_string(std::time(nullptr)), 
                                     type, price, quantity, symbol, "JSON_CLIENT");
    } catch (const std::exception& e) {
        std::cerr << "Error parsing JSON line: " << line << " - " << e.what() << std::endl;
        return nullptr;
    }
}

void DataInterface::on_trade_executed(const Trade& trade) {
    std::lock_guard<std::mutex> lock(statsMutex);
    totalTrades++;
    totalVolume += trade.quantity * trade.price;
    tradeHistory.push_back(trade);
    
    // Keep only last 1000 trades to prevent memory growth
    if (tradeHistory.size() > 1000) {
        tradeHistory.erase(tradeHistory.begin());
    }
}

// dataInterface.hpp
#ifndef DATAINTERFACE_HPP
#define DATAINTERFACE_HPP

#include "order.hpp"
#include "matchingEngine.hpp"
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>

class DataInterface {
public:
    DataInterface(MatchingEngine& engine);
    ~DataInterface();
    
    // File-based data loading
    bool load_orders_from_csv(const std::string& filename);
    bool load_orders_from_json(const std::string& filename);
    
    // Real-time data simulation
    void start_market_data_simulation(const std::string& symbol, double basePrice, int numOrders = 100);
    void stop_simulation();
    
    // Manual order input
    void add_manual_order(OrderType type, double price, int quantity, const std::string& symbol = "DEFAULT");
    
    // Statistics and monitoring
    void print_statistics() const;
    void set_trade_callback(std::function<void(const Trade&)> callback);
    
    // Batch processing
    void process_orders_from_file(const std::string& filename);

private:
    MatchingEngine& matchingEngine;
    std::atomic<bool> simulationRunning;
    std::thread simulationThread;
    std::queue<std::shared_ptr<Order>> orderQueue;
    std::mutex queueMutex;
    std::condition_variable queueCondition;
    
    // Statistics
    mutable std::mutex statsMutex;
    size_t totalTrades = 0;
    double totalVolume = 0.0;
    std::vector<Trade> tradeHistory;
    
    void simulation_worker(const std::string& symbol, double basePrice, int numOrders);
    void process_order_queue();
    std::shared_ptr<Order> parse_csv_line(const std::string& line);
    std::shared_ptr<Order> parse_json_line(const std::string& line);
    void on_trade_executed(const Trade& trade);
};

#endif

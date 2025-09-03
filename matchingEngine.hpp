// matchingEngine.hpp
#ifndef MATCHINGENGINE_HPP
#define MATCHINGENGINE_HPP

#include "order.hpp"
#include "orderBook.hpp"
#include <string>
#include <vector>
#include <functional>

class MatchingEngine {
public:
    using TradeCallback = std::function<void(const Trade&)>;
    
    MatchingEngine();
    
    // Order management
    std::string submit_order(OrderType type, double price, int quantity, 
    const std::string& symbol = "DEFAULT", 
    const std::string& clientId = "DEFAULT");
    bool cancel_order(const std::string& orderId);
    bool modify_order(const std::string& orderId, double newPrice, int newQuantity);
    std::shared_ptr<Order> get_order(const std::string& orderId);
    
    // Market data
    double get_best_bid() const;
    double get_best_ask() const;
    double get_spread() const;
    std::vector<std::pair<double, int>> get_bid_depth(int levels = 5) const;
    std::vector<std::pair<double, int>> get_ask_depth(int levels = 5) const;
    
    // Order book operations
    void print_orderbook() const;
    void set_trade_callback(TradeCallback callback);
    
    // Batch operations for real-time data
    void process_orders_batch(const std::vector<std::shared_ptr<Order>>& orders);
    
    // Statistics
    size_t get_total_orders() const;
    size_t get_active_orders() const;

private:
    OrderBook orderBook;
    std::string generate_order_id();
    void match_order(std::shared_ptr<Order> order);
    int orderCounter;
};

#endif

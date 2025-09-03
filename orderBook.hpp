// orderBook.hpp
#ifndef ORDERBOOK_HPP
#define ORDERBOOK_HPP

#include <map>
#include <unordered_map>
#include <vector>
#include <memory>
#include <functional>
#include "order.hpp"

class OrderBook {
public:
    // Price level -> vector of orders (for price-time priority)
    std::map<double, std::vector<std::shared_ptr<Order>>, std::greater<double>> buyOrders;  // Descending price
    std::map<double, std::vector<std::shared_ptr<Order>>> sellOrders;  // Ascending price
    
    // Order ID -> Order mapping for quick lookup
    std::unordered_map<std::string, std::shared_ptr<Order>> orderMap;
    
    // Market data
    double bestBid = 0.0;
    double bestAsk = 0.0;
    int bidSize = 0;
    int askSize = 0;
    
    // Trade callback function type
    using TradeCallback = std::function<void(const Trade&)>;
    TradeCallback onTrade;

    OrderBook() = default;
    
    void add_order(std::shared_ptr<Order> order);
    bool remove_order(const std::string& orderId);
    bool cancel_order(const std::string& orderId);
    std::shared_ptr<Order> get_order(const std::string& orderId);
    
    // Market data functions
    double get_best_bid() const { return bestBid; }
    double get_best_ask() const { return bestAsk; }
    double get_spread() const { return bestAsk - bestBid; }
    int get_bid_size() const { return bidSize; }
    int get_ask_size() const { return askSize; }
    
    // Get top N levels of market depth
    std::vector<std::pair<double, int>> get_bid_depth(int levels = 5) const;
    std::vector<std::pair<double, int>> get_ask_depth(int levels = 5) const;
    
    // Print order book
    void print_orderbook() const;
    
    // Set trade callback
    void set_trade_callback(TradeCallback callback) { onTrade = callback; }

public:
    void execute_trade(std::shared_ptr<Order> buyOrder, std::shared_ptr<Order> sellOrder, int quantity);

private:
    void update_market_data();
    std::string generate_trade_id();
};

#endif

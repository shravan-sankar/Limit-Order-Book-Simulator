// orderBook.cpp
#include "orderBook.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <random>

void OrderBook::add_order(std::shared_ptr<Order> order) {
    if (!order || order->quantity <= 0) return;
    
    // Store in order map for quick lookup
    orderMap[order->orderId] = order;
    
    if (order->type == BUY) {
        buyOrders[order->price].push_back(order);
    } else {
        sellOrders[order->price].push_back(order);
    }
    
    update_market_data();
}

bool OrderBook::remove_order(const std::string& orderId) {
    auto it = orderMap.find(orderId);
    if (it == orderMap.end()) return false;
    
    auto order = it->second;
    orderMap.erase(it);
    
    if (order->type == BUY) {
        auto priceIt = buyOrders.find(order->price);
        if (priceIt != buyOrders.end()) {
            auto& orders = priceIt->second;
            orders.erase(std::remove_if(orders.begin(), orders.end(),
                [&orderId](const std::shared_ptr<Order>& o) { return o->orderId == orderId; }),
                orders.end());
            
            if (orders.empty()) {
                buyOrders.erase(priceIt);
            }
        }
    } else {
        auto priceIt = sellOrders.find(order->price);
        if (priceIt != sellOrders.end()) {
            auto& orders = priceIt->second;
            orders.erase(std::remove_if(orders.begin(), orders.end(),
                [&orderId](const std::shared_ptr<Order>& o) { return o->orderId == orderId; }),
                orders.end());
            
            if (orders.empty()) {
                sellOrders.erase(priceIt);
            }
        }
    }
    
    update_market_data();
    return true;
}

bool OrderBook::cancel_order(const std::string& orderId) {
    auto it = orderMap.find(orderId);
    if (it == orderMap.end()) return false;
    
    it->second->cancel();
    return remove_order(orderId);
}

std::shared_ptr<Order> OrderBook::get_order(const std::string& orderId) {
    auto it = orderMap.find(orderId);
    return (it != orderMap.end()) ? it->second : nullptr;
}

void OrderBook::update_market_data() {
    bestBid = 0.0;
    bestAsk = 0.0;
    bidSize = 0;
    askSize = 0;
    
    if (!buyOrders.empty()) {
        auto bestBidIt = buyOrders.begin();
        bestBid = bestBidIt->first;
        for (const auto& order : bestBidIt->second) {
            bidSize += order->getRemainingQuantity();
        }
    }
    
    if (!sellOrders.empty()) {
        auto bestAskIt = sellOrders.begin();
        bestAsk = bestAskIt->first;
        for (const auto& order : bestAskIt->second) {
            askSize += order->getRemainingQuantity();
        }
    }
}

std::vector<std::pair<double, int>> OrderBook::get_bid_depth(int levels) const {
    std::vector<std::pair<double, int>> depth;
    int count = 0;
    
    for (const auto& priceLevel : buyOrders) {
        if (count >= levels) break;
        
        int totalSize = 0;
        for (const auto& order : priceLevel.second) {
            totalSize += order->getRemainingQuantity();
        }
        
        depth.emplace_back(priceLevel.first, totalSize);
        count++;
    }
    
    return depth;
}

std::vector<std::pair<double, int>> OrderBook::get_ask_depth(int levels) const {
    std::vector<std::pair<double, int>> depth;
    int count = 0;
    
    for (const auto& priceLevel : sellOrders) {
        if (count >= levels) break;
        
        int totalSize = 0;
        for (const auto& order : priceLevel.second) {
            totalSize += order->getRemainingQuantity();
        }
        
        depth.emplace_back(priceLevel.first, totalSize);
        count++;
    }
    
    return depth;
}

void OrderBook::print_orderbook() const {
    std::cout << "\n=== ORDER BOOK ===" << std::endl;
    std::cout << "Best Bid: " << std::fixed << std::setprecision(2) << bestBid 
              << " (" << bidSize << ")" << std::endl;
    std::cout << "Best Ask: " << std::fixed << std::setprecision(2) << bestAsk 
              << " (" << askSize << ")" << std::endl;
    std::cout << "Spread: " << std::fixed << std::setprecision(2) << get_spread() << std::endl;
    
    std::cout << "\n--- ASK SIDE ---" << std::endl;
    auto askDepth = get_ask_depth(5);
    for (auto it = askDepth.rbegin(); it != askDepth.rend(); ++it) {
        std::cout << std::fixed << std::setprecision(2) << it->first << " | " << it->second << std::endl;
    }
    
    std::cout << "--- BID SIDE ---" << std::endl;
    auto bidDepth = get_bid_depth(5);
    for (const auto& level : bidDepth) {
        std::cout << std::fixed << std::setprecision(2) << level.first << " | " << level.second << std::endl;
    }
    std::cout << "================\n" << std::endl;
}

void OrderBook::execute_trade(std::shared_ptr<Order> buyOrder, std::shared_ptr<Order> sellOrder, int quantity) {
    // Create trade record
    Trade trade(generate_trade_id(), buyOrder->orderId, sellOrder->orderId, 
                buyOrder->symbol, sellOrder->price, quantity);
    
    // Update order quantities
    buyOrder->fill(quantity);
    sellOrder->fill(quantity);
    
    // Remove fully filled orders
    if (buyOrder->isFullyFilled()) {
        remove_order(buyOrder->orderId);
    }
    if (sellOrder->isFullyFilled()) {
        remove_order(sellOrder->orderId);
    }
    
    // Call trade callback if set
    if (onTrade) {
        onTrade(trade);
    }
    
    std::cout << "TRADE: " << quantity << " @ " << std::fixed << std::setprecision(2) 
              << trade.price << " (Trade ID: " << trade.tradeId << ")" << std::endl;
}

std::string OrderBook::generate_trade_id() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(100000, 999999);
    
    std::ostringstream oss;
    oss << "T" << dis(gen);
    return oss.str();
}

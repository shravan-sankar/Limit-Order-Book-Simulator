// order.hpp
#ifndef ORDER_HPP
#define ORDER_HPP

#include <string>
#include <chrono>
#include <memory>

enum OrderType { BUY, SELL };
enum OrderStatus { PENDING, PARTIALLY_FILLED, FILLED, CANCELLED, REJECTED };

class Order {
public:
    std::string orderId;
    OrderType type;
    double price;
    int quantity;
    int filledQuantity;
    OrderStatus status;
    std::chrono::system_clock::time_point timestamp;
    std::string symbol;
    std::string clientId;

    Order(const std::string& id, OrderType t, double p, int q, const std::string& sym = "DEFAULT", const std::string& client = "DEFAULT")
        : orderId(id), type(t), price(p), quantity(q), filledQuantity(0), status(PENDING), 
          timestamp(std::chrono::system_clock::now()), symbol(sym), clientId(client) {}

    bool isFullyFilled() const { return filledQuantity >= quantity; }
    int getRemainingQuantity() const { return quantity - filledQuantity; }
    void fill(int qty) { 
        filledQuantity += qty; 
        if (isFullyFilled()) status = FILLED;
        else if (filledQuantity > 0) status = PARTIALLY_FILLED;
    }
    void cancel() { status = CANCELLED; }
};

struct Trade {
    std::string tradeId;
    std::string buyOrderId;
    std::string sellOrderId;
    std::string symbol;
    double price;
    int quantity;
    std::chrono::system_clock::time_point timestamp;
    
    Trade(const std::string& tid, const std::string& bid, const std::string& sid, 
          const std::string& sym, double p, int q)
        : tradeId(tid), buyOrderId(bid), sellOrderId(sid), symbol(sym), price(p), quantity(q),
          timestamp(std::chrono::system_clock::now()) {}
};

#endif

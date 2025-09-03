#include "matchingEngine.hpp"
#include <algorithm>
#include <iostream>
#include <sstream>

MatchingEngine::MatchingEngine() : orderCounter(0) {
  // Set up trade callback to forward to any external listeners
  orderBook.set_trade_callback([this](const Trade &trade) {
    // Could add additional processing here
  });
}

std::string MatchingEngine::submit_order(OrderType type, double price,
int quantity,
                                         const std::string &symbol,
                                         const std::string &clientId) {
  if (quantity <= 0 || price <= 0) {
    return ""; // Invalid order
  }

  std::string orderId = generate_order_id();
  auto order =
      std::make_shared<Order>(orderId, type, price, quantity, symbol, clientId);

  match_order(order);
  return orderId;
}

bool MatchingEngine::cancel_order(const std::string &orderId) {
  return orderBook.cancel_order(orderId);
}

bool MatchingEngine::modify_order(const std::string &orderId, double newPrice,
                                  int newQuantity) {
  auto order = orderBook.get_order(orderId);
  if (!order || order->status != PENDING) {
    return false;
  }

  // Cancel existing order
  orderBook.cancel_order(orderId);

  // Create new order with modified parameters
  auto newOrder =
      std::make_shared<Order>(orderId, order->type, newPrice, newQuantity,
                              order->symbol, order->clientId);

  match_order(newOrder);
  return true;
}

std::shared_ptr<Order> MatchingEngine::get_order(const std::string &orderId) {
  return orderBook.get_order(orderId);
}

double MatchingEngine::get_best_bid() const { return orderBook.get_best_bid(); }

double MatchingEngine::get_best_ask() const { return orderBook.get_best_ask(); }

double MatchingEngine::get_spread() const { return orderBook.get_spread(); }

std::vector<std::pair<double, int>>
MatchingEngine::get_bid_depth(int levels) const {
  return orderBook.get_bid_depth(levels);
}

std::vector<std::pair<double, int>>
MatchingEngine::get_ask_depth(int levels) const {
  return orderBook.get_ask_depth(levels);
}

void MatchingEngine::print_orderbook() const { orderBook.print_orderbook(); }

void MatchingEngine::set_trade_callback(TradeCallback callback) {
  orderBook.set_trade_callback(callback);
}

void MatchingEngine::process_orders_batch(
    const std::vector<std::shared_ptr<Order>> &orders) {
  for (const auto &order : orders) {
    if (order && order->quantity > 0) {
      match_order(order);
    }
  }
}

size_t MatchingEngine::get_total_orders() const { return orderCounter; }

size_t MatchingEngine::get_active_orders() const {
  // This would need to be tracked separately for efficiency
  // For now, return a placeholder
  return 0;
}

std::string MatchingEngine::generate_order_id() {
  std::ostringstream oss;
  oss << "O" << ++orderCounter;
  return oss.str();
}

void MatchingEngine::match_order(std::shared_ptr<Order> order) {
  if (!order || order->quantity <= 0)
    return;

  if (order->type == BUY) {
    // Match buy order with sell orders (price-time priority)
    while (order->getRemainingQuantity() > 0 && !orderBook.sellOrders.empty()) {
      auto bestAskIt = orderBook.sellOrders.begin();
      if (bestAskIt->first > order->price) {
        break; // No more matching prices
      }

      auto &ordersAtPrice = bestAskIt->second;
      if (ordersAtPrice.empty()) {
        orderBook.sellOrders.erase(bestAskIt);
        continue;
      }

      auto &sellOrder = ordersAtPrice.front();
      int tradeQuantity = std::min(order->getRemainingQuantity(),
                                   sellOrder->getRemainingQuantity());

      // Execute the trade
      orderBook.execute_trade(order, sellOrder, tradeQuantity);

      // Remove fully filled sell order
      if (sellOrder->isFullyFilled()) {
        ordersAtPrice.erase(ordersAtPrice.begin());
        if (ordersAtPrice.empty()) {
          orderBook.sellOrders.erase(bestAskIt);
        }
      }
    }

    // Add remaining quantity to order book
    if (order->getRemainingQuantity() > 0) {
      orderBook.add_order(order);
    }

  } else {
    // Match sell order with buy orders (price-time priority)
    while (order->getRemainingQuantity() > 0 && !orderBook.buyOrders.empty()) {
      auto bestBidIt = orderBook.buyOrders.begin();
      if (bestBidIt->first < order->price) {
        break; // No more matching prices
      }

      auto &ordersAtPrice = bestBidIt->second;
      if (ordersAtPrice.empty()) {
        orderBook.buyOrders.erase(bestBidIt);
        continue;
      }

      auto &buyOrder = ordersAtPrice.front();
      int tradeQuantity = std::min(order->getRemainingQuantity(),
                                   buyOrder->getRemainingQuantity());

      // Execute the trade
      orderBook.execute_trade(buyOrder, order, tradeQuantity);

      // Remove fully filled buy order
      if (buyOrder->isFullyFilled()) {
        ordersAtPrice.erase(ordersAtPrice.begin());
        if (ordersAtPrice.empty()) {
          orderBook.buyOrders.erase(bestBidIt);
        }
      }
    }

    // Add remaining quantity to order book
    if (order->getRemainingQuantity() > 0) {
      orderBook.add_order(order);
    }
  }
}

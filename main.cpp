#include "matchingEngine.hpp"
#include "dataInterface.hpp"
#include "simple_server.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>

int main() {
    std::cout << "=== Limit Order Book Trading System ===" << std::endl;
    
    MatchingEngine engine;
    DataInterface dataInterface(engine);
    SimpleServer server;
    
    // Set up server callbacks
    server.set_matching_engine_callback([&engine](OrderType type, double price, int quantity, const std::string& symbol, const std::string& clientId) {
        return engine.submit_order(type, price, quantity, symbol, clientId);
    });
    
    server.set_cancel_callback([&engine](const std::string& orderId) {
        return engine.cancel_order(orderId);
    });
    
    // Set up trade callback for real-time updates
    engine.set_trade_callback([&server](const Trade& trade) {
        std::cout << "Trade executed: " << trade.quantity << " @ " 
                  << std::fixed << std::setprecision(2) << trade.price 
                  << " (Trade ID: " << trade.tradeId << ")" << std::endl;
        
        // Broadcast trade to all connected clients
        server.broadcast_trade(trade);
    });
    
    // Start server
    std::cout << "\n--- Starting Server ---" << std::endl;
    if (!server.start(8080)) {
        std::cerr << "Failed to start server" << std::endl;
        return 1;
    }
    
    // Run server in background thread
    server.run();
    
    // Demo: Submit some sample orders
    std::cout << "\n--- Submitting Sample Orders ---" << std::endl;
    
    // Add some sell orders first
    std::string sellOrder1 = engine.submit_order(SELL, 100.50, 100, "AAPL", "CLIENT1");
    std::string sellOrder2 = engine.submit_order(SELL, 100.25, 50, "AAPL", "CLIENT2");
    std::string sellOrder3 = engine.submit_order(SELL, 99.75, 75, "AAPL", "CLIENT3");
    
    // Add some buy orders
    std::string buyOrder1 = engine.submit_order(BUY, 100.00, 60, "AAPL", "CLIENT4");
    std::string buyOrder2 = engine.submit_order(BUY, 99.50, 40, "AAPL", "CLIENT5");
    std::string buyOrder3 = engine.submit_order(BUY, 100.30, 80, "AAPL", "CLIENT6");
    
    // Print current order book
    engine.print_orderbook();
    
    // Start market simulation
    std::cout << "\n--- Starting Market Simulation ---" << std::endl;
    dataInterface.start_market_data_simulation("AAPL", 100.00, 20);
    
    std::cout << "\n=== Trading System Ready ===" << std::endl;
    std::cout << "Server running on port 8080" << std::endl;
    std::cout << "Frontend available at: http://localhost:8000" << std::endl;
    std::cout << "Press Enter to stop..." << std::endl;
    
    // Keep the server running
    std::atomic<bool> running(true);
    std::thread marketSimulation([&]() {
        while (running) {
            std::this_thread::sleep_for(std::chrono::seconds(5));
            
            // Broadcast orderbook updates periodically
            server.broadcast_orderbook_update("AAPL", 
                engine.get_best_bid(), 
                engine.get_best_ask(), 
                100, // bid size placeholder
                100  // ask size placeholder
            );
        }
    });
    
    // Wait for user to stop
    std::cin.get();
    running = false;
    
    // Cleanup
    marketSimulation.join();
    dataInterface.stop_simulation();
    server.stop();
    
    std::cout << "\n=== System Shutdown Complete ===" << std::endl;
    return 0;
}

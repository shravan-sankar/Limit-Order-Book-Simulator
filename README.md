# Limit Order Book Trading System

A high-performance limit order book trading system with real-time WebSocket communication between C++ backend and JavaScript frontend.

## Features

- **Real-time Order Matching**: Price-time priority matching engine
- **WebSocket Communication**: Live data streaming between backend and frontend
- **Professional Trading Interface**: TradingView-inspired dark theme UI
- **Order Management**: Submit, cancel, and track orders in real-time
- **Market Data**: Live order book, price charts, and trade history
- **Multi-threaded Architecture**: High-performance concurrent processing

## Architecture

```
┌─────────────────┐    WebSocket    ┌─────────────────┐
│   Frontend      │◄──────────────►│   Backend       │
│   (JavaScript)  │   Port 8080     │   (C++)         │
│   Port 8000     │                 │                 │
└─────────────────┘                 └─────────────────┘
         │                                   │
         │                                   │
    ┌─────────┐                        ┌─────────┐
    │ Browser │                        │Matching │
    │   UI    │                        │ Engine  │
    └─────────┘                        └─────────┘
```

## Quick Start

### 1. Install Dependencies

**macOS:**
```bash
./install.sh
```

**Linux:**
```bash
./install.sh
```

**Manual Installation:**
```bash
# macOS
brew install boost websocketpp nlohmann-json

# Ubuntu/Debian
sudo apt-get install libboost-all-dev libwebsocketpp-dev nlohmann-json3-dev
```

### 2. Build the System

```bash
make clean
make
```

### 3. Run the System

**Terminal 1 - Start Backend:**
```bash
./trading_system
```

**Terminal 2 - Start Frontend:**
```bash
cd frontend
python3 -m http.server 8000
```

### 4. Access the Interface

Open your browser and go to: **http://localhost:8000**

## Usage

### Placing Orders

1. Select a symbol (AAPL, GOOGL, MSFT, TSLA)
2. Choose order type (Buy/Sell)
3. Enter price and quantity
4. Click "Submit Order"

### Real-time Data

- **Order Book**: Live bid/ask prices and sizes
- **Price Chart**: Real-time price movements from actual trades
- **Recent Trades**: Live trade feed
- **Statistics**: Total trades, volume, and averages

### Connection Status

The interface shows connection status:
- 🟢 **Connected**: Real-time data from C++ backend
- 🔴 **Disconnected**: Using simulated data only

## Technical Details

### Backend (C++)

- **Matching Engine**: Price-time priority order matching
- **Order Book**: Efficient data structures for O(log n) operations
- **WebSocket Server**: Real-time communication with frontend
- **Multi-threading**: Concurrent order processing

### Frontend (JavaScript)

- **WebSocket Client**: Real-time data reception
- **Chart.js**: Live price visualization
- **Responsive Design**: TradingView-inspired interface
- **Auto-reconnection**: Handles connection drops

### Data Flow

1. **Order Submission**: Frontend → WebSocket → C++ Backend
2. **Order Matching**: C++ Matching Engine processes orders
3. **Trade Execution**: Real trades generated and broadcast
4. **Live Updates**: WebSocket → Frontend → UI Updates

## API Reference

### WebSocket Messages

**Submit Order:**
```json
{
  "type": "submit_order",
  "orderType": "BUY",
  "price": 150.00,
  "quantity": 100,
  "symbol": "AAPL",
  "clientId": "WEB_CLIENT"
}
```

**Trade Update:**
```json
{
  "type": "trade",
  "tradeId": "T123456",
  "symbol": "AAPL",
  "price": 150.25,
  "quantity": 50,
  "timestamp": 1640995200000
}
```

**Order Book Update:**
```json
{
  "type": "orderbook_update",
  "symbol": "AAPL",
  "bestBid": 150.00,
  "bestAsk": 150.25,
  "bidSize": 100,
  "askSize": 75,
  "spread": 0.25
}
```

## Development

### Project Structure

```
LimitOrderBook/
├── main.cpp                 # Main application entry point
├── matchingEngine.hpp/cpp   # Order matching logic
├── orderBook.hpp/cpp        # Order book data structures
├── order.hpp/cpp           # Order and trade definitions
├── dataInterface.hpp/cpp   # Market data simulation
├── websocket_server.hpp/cpp # WebSocket communication
├── frontend/
│   ├── index.html          # Trading interface
│   ├── styles.css          # TradingView-style CSS
│   └── app.js              # WebSocket client and UI logic
├── Makefile                # Build configuration
└── install.sh              # Installation script
```

### Building from Source

```bash
# Clean previous build
make clean

# Build with debug symbols
make CXXFLAGS="-std=c++17 -Wall -Wextra -g -O0 -pthread"

# Build optimized version
make CXXFLAGS="-std=c++17 -Wall -Wextra -O3 -pthread"
```

### Testing

1. **Backend Testing**: Run `./trading_system` and check console output
2. **Frontend Testing**: Open browser console to see WebSocket messages
3. **Integration Testing**: Place orders and verify real-time updates

## Performance

- **Order Processing**: < 1ms latency for order matching
- **WebSocket Updates**: Real-time data streaming
- **Memory Usage**: Efficient data structures with minimal overhead
- **Concurrency**: Multi-threaded architecture for high throughput

## Troubleshooting

### Common Issues

**WebSocket Connection Failed:**
- Ensure backend is running on port 8080
- Check firewall settings
- Verify WebSocket++ library is installed

**Build Errors:**
- Install all required dependencies
- Use C++17 compatible compiler
- Check library paths

**Frontend Not Loading:**
- Ensure frontend server is running on port 8000
- Check browser console for JavaScript errors
- Verify WebSocket connection status

### Debug Mode

**Backend Debug:**
```bash
make CXXFLAGS="-std=c++17 -Wall -Wextra -g -O0 -pthread -DDEBUG"
./trading_system
```

**Frontend Debug:**
- Open browser developer tools
- Check Console tab for WebSocket messages
- Monitor Network tab for WebSocket connection

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

## License

This project is for educational and demonstration purposes.

## Future Enhancements

- [ ] Advanced order types (Iceberg, Stop orders)
- [ ] Risk management system
- [ ] Database persistence
- [ ] Multiple symbol support
- [ ] Order book depth visualization
- [ ] Performance analytics dashboard
- [ ] REST API endpoints
- [ ] Authentication and authorization

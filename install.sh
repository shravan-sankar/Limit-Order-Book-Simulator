#!/bin/bash

echo "=== Limit Order Book Trading System Installation ==="

# Check if we're on macOS or Linux
if [[ "$OSTYPE" == "darwin"* ]]; then
    echo "Detected macOS"
    
    # Check if Homebrew is installed
    if ! command -v brew &> /dev/null; then
        echo "Installing Homebrew..."
        /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    fi
    
    echo "Installing dependencies with Homebrew..."
    brew install boost websocketpp nlohmann-json
    
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    echo "Detected Linux"
    
    # Update package list
    sudo apt-get update
    
    echo "Installing dependencies with apt..."
    sudo apt-get install -y \
        build-essential \
        libboost-all-dev \
        libwebsocketpp-dev \
        nlohmann-json3-dev \
        cmake
    
else
    echo "Unsupported operating system: $OSTYPE"
    echo "Please install the following dependencies manually:"
    echo "- Boost libraries"
    echo "- WebSocket++"
    echo "- nlohmann/json"
    exit 1
fi

echo "Building the trading system..."
make clean
make

if [ $? -eq 0 ]; then
    echo ""
    echo "=== Installation Complete! ==="
    echo ""
    echo "To run the system:"
    echo "1. Start the backend:  ./trading_system"
    echo "2. Start the frontend: cd frontend && python3 -m http.server 8000"
    echo "3. Open browser:       http://localhost:8000"
    echo ""
    echo "The WebSocket server will run on port 8080"
    echo "The web frontend will run on port 8000"
else
    echo "Build failed. Please check the error messages above."
    exit 1
fi

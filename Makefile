# Makefile for Limit Order Book Trading System

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -pthread
INCLUDES = -I/opt/homebrew/include -I/usr/local/include -I/usr/include
LIBS = -lpthread

# Source files
SOURCES = main.cpp matchingEngine.cpp orderBook.cpp order.cpp dataInterface.cpp simple_server.cpp
OBJECTS = $(SOURCES:.cpp=.o)
TARGET = trading_system

# Default target
all: $(TARGET)

# Build the main executable
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS) $(LIBS)

# Compile source files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Clean build files
clean:
	rm -f $(OBJECTS) $(TARGET)

# Install dependencies (Ubuntu/Debian)
install-deps:
	sudo apt-get update
	sudo apt-get install -y libwebsocketpp-dev libboost-all-dev nlohmann-json3-dev

# Install dependencies (macOS)
install-deps-mac:
	brew install boost websocketpp nlohmann-json

# Run the system
run: $(TARGET)
	./$(TARGET)

# Run frontend
run-frontend:
	cd frontend && python3 -m http.server 8000

# Help
help:
	@echo "Available targets:"
	@echo "  all          - Build the trading system"
	@echo "  clean        - Remove build files"
	@echo "  install-deps - Install dependencies (Ubuntu/Debian)"
	@echo "  install-deps-mac - Install dependencies (macOS)"
	@echo "  run          - Run the trading system"
	@echo "  run-frontend - Run the frontend web server"
	@echo "  help         - Show this help message"

.PHONY: all clean install-deps install-deps-mac run run-frontend help

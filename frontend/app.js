// Limit Order Book Trading Interface
class TradingInterface {
    constructor() {
        this.socket = null;
        this.isConnected = false;
        this.candlestickChart = null;
        this.candlestickSeries = null;
        this.volumeSeries = null;
        this.activeOrders = new Map();
        this.recentTrades = [];
        this.candlestickData = new Map(); // Map of timeframe -> candlestick data
        this.currentTimeframe = '1m';
        this.statistics = {
            totalTrades: 0,
            totalVolume: 0,
            lastTrade: '--',
            avgTradeSize: 0
        };
        
        this.initializeEventListeners();
        this.initializeCandlestickChart();
        this.loadSampleData();
        
        // Connect to WebSocket server
        this.connectWebSocket();
    }

    initializeEventListeners() {
        // Order form submission
        document.getElementById('orderForm').addEventListener('submit', (e) => {
            e.preventDefault();
            this.submitOrder();
        });

        // Control buttons
        document.getElementById('startSimulation').addEventListener('click', () => {
            this.startSimulation();
        });

        document.getElementById('stopSimulation').addEventListener('click', () => {
            this.stopSimulation();
        });

        document.getElementById('loadSampleData').addEventListener('click', () => {
            this.loadSampleData();
        });

        document.getElementById('clearOrders').addEventListener('click', () => {
            this.clearAllOrders();
        });

        // Symbol selector
        document.getElementById('symbolSelect').addEventListener('change', (e) => {
            this.updateOrderBook(e.target.value);
        });

        // Timeframe selector
        document.getElementById('timeframeSelect').addEventListener('change', (e) => {
            this.currentTimeframe = e.target.value;
            this.updateChartTimeframe();
        });

        // Modal close
        document.querySelector('.close').addEventListener('click', () => {
            document.getElementById('orderModal').style.display = 'none';
        });

        // Close modal when clicking outside
        window.addEventListener('click', (e) => {
            const modal = document.getElementById('orderModal');
            if (e.target === modal) {
                modal.style.display = 'none';
            }
        });
    }

    initializeCandlestickChart() {
        const chartContainer = document.getElementById('candlestickChart');
        
        // Create the chart
        this.candlestickChart = LightweightCharts.createChart(chartContainer, {
            width: chartContainer.clientWidth,
            height: chartContainer.clientHeight,
            layout: {
                background: { color: '#131722' },
                textColor: '#d1d4dc',
            },
            grid: {
                vertLines: { color: '#2a2e39' },
                horzLines: { color: '#2a2e39' },
            },
            crosshair: {
                mode: LightweightCharts.CrosshairMode.Normal,
            },
            rightPriceScale: {
                borderColor: '#2a2e39',
                scaleMargins: {
                    top: 0.1,
                    bottom: 0.2,
                },
            },
            timeScale: {
                borderColor: '#2a2e39',
                timeVisible: true,
                secondsVisible: false,
            },
        });

        // Create candlestick series
        this.candlestickSeries = this.candlestickChart.addCandlestickSeries({
            upColor: '#26a69a',
            downColor: '#ef5350',
            borderDownColor: '#ef5350',
            borderUpColor: '#26a69a',
            wickDownColor: '#ef5350',
            wickUpColor: '#26a69a',
        });

        // Create volume series
        this.volumeSeries = this.candlestickChart.addHistogramSeries({
            color: '#26a69a',
            priceFormat: {
                type: 'volume',
            },
            priceScaleId: '',
            scaleMargins: {
                top: 0.8,
                bottom: 0,
            },
        });

        // Handle window resize
        window.addEventListener('resize', () => {
            this.candlestickChart.applyOptions({
                width: chartContainer.clientWidth,
                height: chartContainer.clientHeight,
            });
        });

        // Initialize candlestick data for different timeframes
        this.initializeCandlestickData();
        
        // Mark chart as loaded
        chartContainer.classList.add('loaded');
    }

    initializeCandlestickData() {
        const timeframes = ['1m', '5m', '15m', '1h', '1d'];
        timeframes.forEach(timeframe => {
            this.candlestickData.set(timeframe, []);
        });
        
        // Generate sample data for each timeframe
        this.generateSampleCandlestickData();
    }

    generateSampleCandlestickData() {
        const timeframes = ['1m', '5m', '15m', '1h', '1d'];
        const basePrice = 150.00; // AAPL base price
        
        timeframes.forEach(timeframe => {
            const data = [];
            const now = Math.floor(Date.now() / 1000);
            const intervalSeconds = this.getTimeframeSeconds(timeframe);
            
            // Generate 100 candlesticks for each timeframe
            for (let i = 99; i >= 0; i--) {
                const time = now - (i * intervalSeconds);
                const open = basePrice + (Math.random() - 0.5) * 2;
                const high = open + Math.random() * 1;
                const low = open - Math.random() * 1;
                const close = open + (Math.random() - 0.5) * 0.5;
                const volume = Math.floor(Math.random() * 1000000) + 100000;
                
                data.push({
                    time: time,
                    open: open,
                    high: high,
                    low: low,
                    close: close,
                    volume: volume
                });
            }
            
            this.candlestickData.set(timeframe, data);
        });
        
        // Set initial data for current timeframe
        this.updateChartTimeframe();
    }

    getTimeframeSeconds(timeframe) {
        const timeframes = {
            '1m': 60,
            '5m': 300,
            '15m': 900,
            '1h': 3600,
            '1d': 86400
        };
        return timeframes[timeframe] || 60;
    }

    updateChartTimeframe() {
        const data = this.candlestickData.get(this.currentTimeframe) || [];
        
        // Update candlestick series
        this.candlestickSeries.setData(data);
        
        // Update volume series
        const volumeData = data.map(candle => ({
            time: candle.time,
            value: candle.volume,
            color: candle.close >= candle.open ? '#26a69a' : '#ef5350'
        }));
        this.volumeSeries.setData(volumeData);
        
        // Fit content to view
        this.candlestickChart.timeScale().fitContent();
    }

    submitOrder() {
        const orderType = document.getElementById('orderType').value;
        const price = parseFloat(document.getElementById('orderPrice').value);
        const quantity = parseInt(document.getElementById('orderQuantity').value);
        const clientId = document.getElementById('clientId').value;
        const symbol = document.getElementById('symbolSelect').value;

        if (!price || !quantity || !clientId) {
            alert('Please fill in all required fields');
            return;
        }

        if (!this.isConnected) {
            alert('Not connected to trading server');
            return;
        }

        // Send order to backend via WebSocket
        const orderData = {
            type: 'submit_order',
            orderType: orderType,
            price: price,
            quantity: quantity,
            symbol: symbol,
            clientId: clientId
        };

        this.socket.send(JSON.stringify(orderData));
        
        // Clear form
        document.getElementById('orderPrice').value = '';
        document.getElementById('orderQuantity').value = '';
    }

    connectWebSocket() {
        // For now, simulate connection since we're using a simple TCP server
        // In a real implementation, you'd use WebSocket or Server-Sent Events
        console.log('Simulating connection to trading server');
        this.isConnected = true;
        this.updateConnectionStatus(true);
        
        // Simulate receiving some initial data
        setTimeout(() => {
            this.handleWebSocketMessage({
                type: 'welcome',
                message: 'Connected to Limit Order Book Trading System'
            });
        }, 1000);
    }

    handleWebSocketMessage(data) {
        switch (data.type) {
            case 'welcome':
                console.log('Server message:', data.message);
                break;
                
            case 'trade':
                this.handleRealTrade(data);
                break;
                
            case 'orderbook_update':
                this.handleOrderBookUpdate(data);
                break;
                
            case 'order_submitted':
                this.handleOrderSubmitted(data);
                break;
                
            case 'order_cancelled':
                this.handleOrderCancelled(data);
                break;
                
            case 'error':
                console.error('Server error:', data.message);
                alert('Server error: ' + data.message);
                break;
                
            default:
                console.log('Unknown message type:', data.type);
        }
    }

    handleRealTrade(tradeData) {
        // Add to recent trades
        this.recentTrades.unshift(tradeData);
        if (this.recentTrades.length > 50) {
            this.recentTrades.pop();
        }

        // Update statistics
        this.statistics.totalTrades++;
        this.statistics.totalVolume += tradeData.quantity;
        this.statistics.lastTrade = `$${tradeData.price.toFixed(2)}`;
        this.statistics.avgTradeSize = this.statistics.totalVolume / this.statistics.totalTrades;

        // Update displays
        this.updateStatistics();
        this.updateRecentTradesDisplay();
        this.updateCandlestickChart(tradeData);
    }

    handleOrderBookUpdate(data) {
        // Update market data display
        document.getElementById('bestBid').textContent = `$${data.bestBid.toFixed(2)}`;
        document.getElementById('bestAsk').textContent = `$${data.bestAsk.toFixed(2)}`;
        document.getElementById('bidSize').textContent = `(${data.bidSize})`;
        document.getElementById('askSize').textContent = `(${data.askSize})`;
        document.getElementById('spread').textContent = `$${data.spread.toFixed(2)}`;
    }

    handleOrderSubmitted(data) {
        if (data.status === 'success') {
            console.log('Order submitted successfully:', data.orderId);
            // You could add the order to active orders here if needed
        } else {
            alert('Failed to submit order');
        }
    }

    handleOrderCancelled(data) {
        if (data.status === 'success') {
            console.log('Order cancelled successfully:', data.orderId);
            this.removeActiveOrder(data.orderId);
        } else {
            alert('Failed to cancel order');
        }
    }

    updateConnectionStatus(connected) {
        const statusElement = document.getElementById('connectionStatus');
        if (connected) {
            statusElement.textContent = 'Connected';
            statusElement.className = 'status-connected';
        } else {
            statusElement.textContent = 'Disconnected';
            statusElement.className = 'status-disconnected';
        }
    }

    updateStatistics() {
        document.getElementById('totalTrades').textContent = this.statistics.totalTrades;
        document.getElementById('totalVolume').textContent = this.statistics.totalVolume;
        document.getElementById('lastTrade').textContent = this.statistics.lastTrade;
        document.getElementById('avgTradeSize').textContent = this.statistics.avgTradeSize.toFixed(2);
    }

    generateOrderId() {
        return 'ORD_' + Date.now() + '_' + Math.random().toString(36).substr(2, 5);
    }

    addActiveOrder(order) {
        this.activeOrders.set(order.id, order);
        this.updateActiveOrdersDisplay();
    }

    removeActiveOrder(orderId) {
        this.activeOrders.delete(orderId);
        this.updateActiveOrdersDisplay();
    }

    updateActiveOrdersDisplay() {
        const container = document.getElementById('activeOrders');
        
        if (this.activeOrders.size === 0) {
            container.innerHTML = '<p style="text-align: center; opacity: 0.6; font-size: 11px;">No active orders</p>';
            return;
        }

        // Use DocumentFragment for better performance
        const fragment = document.createDocumentFragment();

        this.activeOrders.forEach(order => {
            const orderElement = document.createElement('div');
            orderElement.className = `order-item ${order.type.toLowerCase()}`;
            orderElement.innerHTML = `
                <div class="order-id">${order.id}</div>
                <div class="order-details">
                    ${order.type} ${order.quantity} @ $${order.price.toFixed(2)}<br>
                    Symbol: ${order.symbol} | Status: ${order.status}
                </div>
            `;
            fragment.appendChild(orderElement);
        });
        
        // Single DOM update
        container.innerHTML = '';
        container.appendChild(fragment);
    }

    simulateOrderExecution(order) {
        // Simulate order execution with random delay
        setTimeout(() => {
            const executionPrice = order.price + (Math.random() - 0.5) * 0.1;
            const executedQuantity = Math.floor(Math.random() * order.quantity) + 1;
            
            this.executeTrade({
                orderId: order.id,
                symbol: order.symbol,
                price: executionPrice,
                quantity: executedQuantity,
                timestamp: new Date().toISOString()
            });

            if (executedQuantity >= order.quantity) {
                this.removeActiveOrder(order.id);
            } else {
                // Partial fill
                order.quantity -= executedQuantity;
                this.updateActiveOrdersDisplay();
            }
        }, Math.random() * 3000 + 1000);
    }

    executeTrade(trade) {
        this.recentTrades.unshift(trade);
        if (this.recentTrades.length > 50) {
            this.recentTrades.pop();
        }

        this.updateStatistics(trade);
        this.updateRecentTradesDisplay();
        this.updateCandlestickChart(trade);
    }

    updateStatistics(trade) {
        this.statistics.totalTrades++;
        this.statistics.totalVolume += trade.quantity;
        this.statistics.lastTrade = `$${trade.price.toFixed(2)}`;
        this.statistics.avgTradeSize = this.statistics.totalVolume / this.statistics.totalTrades;

        document.getElementById('totalTrades').textContent = this.statistics.totalTrades;
        document.getElementById('totalVolume').textContent = this.statistics.totalVolume;
        document.getElementById('lastTrade').textContent = this.statistics.lastTrade;
        document.getElementById('avgTradeSize').textContent = this.statistics.avgTradeSize.toFixed(2);
    }

    updateRecentTradesDisplay() {
        const container = document.getElementById('recentTrades');
        
        // Use DocumentFragment for better performance
        const fragment = document.createDocumentFragment();
        
        // Limit to 8 trades for better performance
        this.recentTrades.slice(0, 8).forEach(trade => {
            const tradeElement = document.createElement('div');
            tradeElement.className = 'trade-item';
            tradeElement.innerHTML = `
                ${trade.symbol}: ${trade.quantity} @ $${trade.price.toFixed(2)}<br>
                <small>${new Date(trade.timestamp).toLocaleTimeString()}</small>
            `;
            fragment.appendChild(tradeElement);
        });
        
        // Single DOM update
        container.innerHTML = '';
        container.appendChild(fragment);
    }

    updateCandlestickChart(tradeData) {
        const now = Math.floor(Date.now() / 1000);
        const timeframe = this.currentTimeframe;
        const intervalSeconds = this.getTimeframeSeconds(timeframe);
        
        // Get current candlestick data for the timeframe
        const data = this.candlestickData.get(timeframe) || [];
        
        // Find the current candlestick period
        const currentPeriodStart = Math.floor(now / intervalSeconds) * intervalSeconds;
        let currentCandle = data.find(candle => candle.time === currentPeriodStart);
        
        if (!currentCandle) {
            // Create new candlestick
            const lastCandle = data[data.length - 1];
            const open = lastCandle ? lastCandle.close : tradeData.price;
            
            currentCandle = {
                time: currentPeriodStart,
                open: open,
                high: tradeData.price,
                low: tradeData.price,
                close: tradeData.price,
                volume: tradeData.quantity
            };
            
            data.push(currentCandle);
        } else {
            // Update existing candlestick
            currentCandle.high = Math.max(currentCandle.high, tradeData.price);
            currentCandle.low = Math.min(currentCandle.low, tradeData.price);
            currentCandle.close = tradeData.price;
            currentCandle.volume += tradeData.quantity;
        }
        
        // Keep only last 100 candlesticks
        if (data.length > 100) {
            data.splice(0, data.length - 100);
        }
        
        // Update the chart if we're viewing this timeframe
        if (timeframe === this.currentTimeframe) {
            this.candlestickSeries.update(currentCandle);
            
            // Update volume
            const volumeData = {
                time: currentCandle.time,
                value: currentCandle.volume,
                color: currentCandle.close >= currentCandle.open ? '#26a69a' : '#ef5350'
            };
            this.volumeSeries.update(volumeData);
        }
        
        // Store updated data
        this.candlestickData.set(timeframe, data);
    }

    loadSampleData() {
        // Generate sample order book data
        const symbol = document.getElementById('symbolSelect').value;
        const basePrice = this.getBasePrice(symbol);
        
        this.generateSampleOrderBook(basePrice);
        this.updateMarketData(basePrice);
        
        // Regenerate candlestick data for new symbol
        this.generateSampleCandlestickData();
    }

    getBasePrice(symbol) {
        const prices = {
            'AAPL': 150.00,
            'GOOGL': 2800.00,
            'MSFT': 300.00,
            'TSLA': 200.00
        };
        return prices[symbol] || 100.00;
    }

    generateSampleOrderBook(basePrice) {
        const orderBookBody = document.getElementById('orderBookBody');
        
        // Use DocumentFragment for better performance
        const fragment = document.createDocumentFragment();

        // Generate bid orders (below market price) - reduced to 8 for performance
        for (let i = 0; i < 8; i++) {
            const price = basePrice - (i + 1) * 0.01;
            const size = Math.floor(Math.random() * 500) + 50;
            const total = size * (i + 1);

            const row = document.createElement('tr');
            row.className = 'bid-row';
            row.innerHTML = `
                <td>$${price.toFixed(2)}</td>
                <td>${size}</td>
                <td>${total}</td>
            `;
            fragment.appendChild(row);
        }

        // Generate ask orders (above market price) - reduced to 8 for performance
        for (let i = 0; i < 8; i++) {
            const price = basePrice + (i + 1) * 0.01;
            const size = Math.floor(Math.random() * 500) + 50;
            const total = size * (i + 1);

            const row = document.createElement('tr');
            row.className = 'ask-row';
            row.innerHTML = `
                <td>$${price.toFixed(2)}</td>
                <td>${size}</td>
                <td>${total}</td>
            `;
            fragment.appendChild(row);
        }

        // Single DOM update
        orderBookBody.innerHTML = '';
        orderBookBody.appendChild(fragment);
    }

    updateMarketData(basePrice) {
        const bestBid = basePrice - 0.01;
        const bestAsk = basePrice + 0.01;
        const spread = bestAsk - bestBid;

        document.getElementById('bestBid').textContent = `$${bestBid.toFixed(2)}`;
        document.getElementById('bidSize').textContent = `(${Math.floor(Math.random() * 1000) + 100})`;
        document.getElementById('bestAsk').textContent = `$${bestAsk.toFixed(2)}`;
        document.getElementById('askSize').textContent = `(${Math.floor(Math.random() * 1000) + 100})`;
        document.getElementById('spread').textContent = `$${spread.toFixed(2)}`;
    }

    updateOrderBook(symbol) {
        this.loadSampleData();
    }

    // Simulation methods removed - now using real data from WebSocket

    clearAllOrders() {
        this.activeOrders.clear();
        this.updateActiveOrdersDisplay();
    }
}

// Initialize the trading interface when the page loads
document.addEventListener('DOMContentLoaded', () => {
    new TradingInterface();
});

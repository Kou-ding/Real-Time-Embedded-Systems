Invidia, Apple, Tesla, Amazon
Bitcoin, Ethereum, Monero

Every minute compute the (candlestick):
- first value
- last value
- max value
- min value
- total volume
- the moving mean value of the exchanges of the last 15 minutes.

The report should be 4 pages containing:
- the implementation method
- a diagram of the time differences
- a diagram of the percentage of the time the CPU remains idle 
- comments and results that show that the program can function for days writing data without losses and without stopping if something happens to the connection or the 

### WebSockets | Part 1

#### Finnhub account
First, we have to make an account in [Finnhub](https://finnhub.io) so that we can get an API key. Finnhub is a financial API organized around REST and the key it gives us is used to authenticate our specific requests.

#### Python example
To test the functionality of the web socket we run a Python script after activating the correct environment that contains the necessary dependencies.
```bash
# To create a virtual environment
sudo python3 -m venv vwebsockets
# To activate the virtual environment
source vwebsockets/bin/activate
# Installing the necessary dependencies
pip install websocket finnhub-python websocket-client
# To deactivate it
deactivate
# To run the code
python3 finnhub.py
```
What we receive for currently traded markets is(Bitcoin):
```
++Rcv raw: b'\x81~\x03\x0f{"data":[{"c":null,"p":66972.77,"s":"BINANCE:BTCUSDT","t":1721555886208,"v":0.01559},{"c":null,"p":66972.77,"s":"BINANCE:BTCUSDT","t":1721555886208,"v":0.00008},{"c":null,"p":66972.77,"s":"BINANCE:BTCUSDT","t":1721555886208,"v":0.0001},{"c":null,"p":66972.77,"s":"BINANCE:BTCUSDT","t":1721555886208,"v":0.02423},{"c":null,"p":66972.77,"s":"BINANCE:BTCUSDT","t":1721555886261,"v":0.00011},{"c":null,"p":66972.77,"s":"BINANCE:BTCUSDT","t":1721555886418,"v":0.00084},{"c":null,"p":66972.77,"s":"BINANCE:BTCUSDT","t":1721555886721,"v":0.00746},{"c":null,"p":66972.77,"s":"BINANCE:BTCUSDT","t":1721555886721,"v":0.00149},{"c":null,"p":66972.78,"s":"BINANCE:BTCUSDT","t":1721555886860,"v":0.0208},{"c":null,"p":66972.77,"s":"BINANCE:BTCUSDT","t":1721555886950,"v":0.00122}],"type":"trade"}'

++Rcv decoded: fin=1 opcode=1 data=b'{"data":[{"c":null,"p":66972.77,"s":"BINANCE:BTCUSDT","t":1721555886208,"v":0.01559},{"c":null,"p":66972.77,"s":"BINANCE:BTCUSDT","t":1721555886208,"v":0.00008},{"c":null,"p":66972.77,"s":"BINANCE:BTCUSDT","t":1721555886208,"v":0.0001},{"c":null,"p":66972.77,"s":"BINANCE:BTCUSDT","t":1721555886208,"v":0.02423},{"c":null,"p":66972.77,"s":"BINANCE:BTCUSDT","t":1721555886261,"v":0.00011},{"c":null,"p":66972.77,"s":"BINANCE:BTCUSDT","t":1721555886418,"v":0.00084},{"c":null,"p":66972.77,"s":"BINANCE:BTCUSDT","t":1721555886721,"v":0.00746},{"c":null,"p":66972.77,"s":"BINANCE:BTCUSDT","t":1721555886721,"v":0.00149},{"c":null,"p":66972.78,"s":"BINANCE:BTCUSDT","t":1721555886860,"v":0.0208},{"c":null,"p":66972.77,"s":"BINANCE:BTCUSDT","t":1721555886950,"v":0.00122}],"type":"trade"}
```
What we receive for not-currently traded markets is(Apple):
```
++Rcv raw: b'\x81\x0f{"type":"ping"}'

++Rcv decoded: fin=1 opcode=1 data=b'{"type":"ping"}'
{"type":"ping"}
```
The operating schedule of cryptocurrencies is 24/7 while normal stocks have certain market hours depending on the stock exchange they are on.

Type: ping, signifies that the connection has been established though there are no market data to send.
Type: trade, observes the market and based on the s: symbol it is provided includes useful information such as:
- p: last price, 
- v: volume,
- t: unix milliseconds timestamp,
- c: list of trade conditions
### Pthreads | Part 2
Now that we know how Finnhub works and what kind of data it provides we need to do the same thing but in C and parallelize the process using pthreads.

#### Installing web sockets C library
Then, we need to install a library that will help us make these necessary "GET" requests, effectively asking the current market's value for certain stocks and/or cryptos.
For Debian-based distributions, the command to install the WebSocket library is as follows:
```bash
#to get the library via your distro's package manager
sudo apt install libwebsockets-dev
```
or
```bash
#to build the library yourself
git clone https://libwebsockets.org/repo/libwebsockets
cd libwebsockets
mkdir build
cd build
cmake ..
make
sudo make install
```
Usually building it yourself guarantees that the latest version of the library is being utilized.

#### Understanding Web Sockets
The C implementation helps us understand the fundamentals of a web socket connection. These are as follows:
- Step 1
    - Send an http request to the server asking to open a connection
- Step 2
    - The server sends a response to the client 101 Switching Protocols
- Step 3
    - The channel is now open for bidirectional communication.

In our case, we subscribe to certain Stocks and receive the same information as the Python script we tried earlier.

### Results and Observations | Part 3



### Sources
- https://stackoverflow.com/questions/30904560/libwebsocket-client-example
- https://finnhub.io/docs/api/library
- https://github.com/Finnhub-Stock-API/finnhub-python
- 
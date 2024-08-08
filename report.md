### Introduction
In this assignment, we are going to be opening a WebSocket connection to receive and process stock and crypto data. We are going to be keeping track of the following stocks:
- Invidia,
- Google
and the following crypto:
- Bitcoin,
- Ethereum.

Stocks and crypto have differing operating hours and that is why I chose to incorporate, in addition to stocks that are exchanged on normal stock market hours, two very popular cryptocurrencies that are being exchanged 24/7. This setup will provide us with a constant influx of sufficient data to process.

### Assignment Requirements
Log all past stock exchanges in a separate file for each stock respectively.

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
- comments and results that show that the program can function for days writing data without losses and without stopping if something happens to the connection or the network.

### WebSockets & Python | Part 1

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
What we receive for currently traded markets (in our case: Bitcoin) is:
```
++Rcv raw: b'\x81~\x03\x0f{"data":[{"c":null,"p":66972.77,"s":"BINANCE:BTCUSDT","t":1721555886208,"v":0.01559},{"c":null,"p":66972.77,"s":"BINANCE:BTCUSDT","t":1721555886208,"v":0.00008},{"c":null,"p":66972.77,"s":"BINANCE:BTCUSDT","t":1721555886208,"v":0.0001},{"c":null,"p":66972.77,"s":"BINANCE:BTCUSDT","t":1721555886208,"v":0.02423},{"c":null,"p":66972.77,"s":"BINANCE:BTCUSDT","t":1721555886261,"v":0.00011},{"c":null,"p":66972.77,"s":"BINANCE:BTCUSDT","t":1721555886418,"v":0.00084},{"c":null,"p":66972.77,"s":"BINANCE:BTCUSDT","t":1721555886721,"v":0.00746},{"c":null,"p":66972.77,"s":"BINANCE:BTCUSDT","t":1721555886721,"v":0.00149},{"c":null,"p":66972.78,"s":"BINANCE:BTCUSDT","t":1721555886860,"v":0.0208},{"c":null,"p":66972.77,"s":"BINANCE:BTCUSDT","t":1721555886950,"v":0.00122}],"type":"trade"}'

++Rcv decoded: fin=1 opcode=1 data=b'{"data":[{"c":null,"p":66972.77,"s":"BINANCE:BTCUSDT","t":1721555886208,"v":0.01559},{"c":null,"p":66972.77,"s":"BINANCE:BTCUSDT","t":1721555886208,"v":0.00008},{"c":null,"p":66972.77,"s":"BINANCE:BTCUSDT","t":1721555886208,"v":0.0001},{"c":null,"p":66972.77,"s":"BINANCE:BTCUSDT","t":1721555886208,"v":0.02423},{"c":null,"p":66972.77,"s":"BINANCE:BTCUSDT","t":1721555886261,"v":0.00011},{"c":null,"p":66972.77,"s":"BINANCE:BTCUSDT","t":1721555886418,"v":0.00084},{"c":null,"p":66972.77,"s":"BINANCE:BTCUSDT","t":1721555886721,"v":0.00746},{"c":null,"p":66972.77,"s":"BINANCE:BTCUSDT","t":1721555886721,"v":0.00149},{"c":null,"p":66972.78,"s":"BINANCE:BTCUSDT","t":1721555886860,"v":0.0208},{"c":null,"p":66972.77,"s":"BINANCE:BTCUSDT","t":1721555886950,"v":0.00122}],"type":"trade"}
```
What we receive for not-currently traded markets (in our case: Apple) is:
```
++Rcv raw: b'\x81\x0f{"type":"ping"}'

++Rcv decoded: fin=1 opcode=1 data=b'{"type":"ping"}'
{"type":"ping"}
```
The operating schedule of cryptocurrencies is 24/7 while normal stocks have certain market hours depending on the stock exchange they are on.

>Type: ping, signifies that the connection has been established though there are no market data to send.

>Type: trade, contains market data relevant to the symbols we subscribe to

The trade information being provided has this form:
- s: symbol
- p: last price, 
- v: volume,
- t: unix milliseconds timestamp,
- c: list of trade conditions

### Websockets & C lang | Part 2
Now that we know how Finnhub works and what kind of data it provides we need to do the same thing but in C and parallelize the process using pthreads.
> Note: The code has a lot of comments that go into more specific parts of the application.

#### Installing web sockets C library
Then, we need to install a library that will help us subscribe to certain market symbols, effectively listening to the current market's activity for certain stocks and/or cryptos.
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
    - Send an HTTP request to the server asking to open a connection
- Step 2
    - The server sends a response to the client 101 Switching Protocols
- Step 3
    - The channel is now open for bidirectional communication.

Steps 1 and 2 are considered the handshake procedure and in our case, we subscribe to certain Stocks and receive the same information as the Python script we tried earlier.

#### Libwebsocket code
Let's translate the previously stated WebSocket theory into actual c code.
The gist of the program's logic is within the WebSocket service callback function. A callback function is designed to be called by another function. In our case, this stands true as it gets called on different occasions:
- When creating the context of the connection the protocols array is passed as a parameter. This array contains, among others, the callback function. 
- And every time the main while loop comes around in order for the WebSocket to continue serving its purpose. Specifically, this function utilizes it: lws_service(context, 500).

The aforementioned callback function contains all the possible cases a connection needs:
- The Client established a connection
- Connection error
- Connection closed
- The Client received a message
- Client writeable

When the connection is established the callback function runs the "connection established case". This changes a public flag to let us know of the connection's updated state and also prints it for the user to see.

Before receiving a message the program must first succeed in a handshake with the server. This is done inside the Client writeable case where we pass on the subscription symbols to the server to then later receive all the relevant information. 

After all that is done, we can now move on to receiving the message via the "the client received a message" case of the callback function. Here we analyze the JSON formatted message that is sent through the WebSocket connection to extract the data and log it inside a txt file. The candlestick is also calculated here.

#### P threads 
Finally, we need to parallelize the c code using p threads so that we meet the real-time demands of our application. We will be assigning one producer and one consumer to each stock. They are going to be making use of a common circular queue unique for each stock. The producer function adds trade data to the queue and the consumer takes them out in due time to calculate the candlestick and store some trade data in a txt file as a failsafe for time when the system is unstable. We utilize a mutex so that only one thread can give or take from the queue at a particular moment in time. Finally, we set some variable arrays, that have as many elements as there are threads, so that we can have unique variables for each thread to work with and calculate times, values and everything that is needed.

### Results | Part 3

### Observations | Part 4
When setting up the protocol array I originally set the buffer size at 1000. This allowed me to read the entirety of the messages being sent, but once in a while, the connection would terminate without any obvious, for me, reason. Setting the buffer to 0 resolved the issue.

There seems to be an occurrence where the WebSocket connection terminates on its own around the 2-hour mark. The connection flag remains 1, indicating that the connection is still open when in reality we are not receiving any data. This makes it difficult for a 


### Sources
- https://stackoverflow.com/questions/30904560/libwebsocket-client-example
- https://finnhub.io/docs/api/library
- https://github.com/Finnhub-Stock-API/finnhub-python
- 
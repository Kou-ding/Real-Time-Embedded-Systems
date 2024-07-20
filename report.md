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
First, we get an API key from [Finnhub](https://finnhub.io). Finnhub is a financial API organized around REST and the key it gives us is used to authenticate our specific requests.

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
### Pthreads | Part 2
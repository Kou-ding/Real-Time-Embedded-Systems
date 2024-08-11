#https://pypi.org/project/websocket_client/
import websocket

# determines program behaviour when there is a websocket message
def on_message(ws, message):
    print(message)

# determines program behaviour when there is a websocket error
def on_error(ws, error):
    print(error)

# determines program behaviour when the websocket closes
def on_close(ws, close_status_code, close_msg):
    print("### closed ###")
    print("Close status code:", close_status_code)
    print("Close message:", close_msg)

# determines program behaviour when the websocket has successfully opened
def on_open(ws):
    #ws.send('{"type":"subscribe","symbol":"TM"}') #Toyota
    ws.send('{"type":"subscribe","symbol":"NVDA"}') #Nvidia
    #ws.send('{"type":"subscribe","symbol":"AMD"}') #AMD
    ws.send('{"type":"subscribe","symbol":"GOOGL"}') #Google
    #ws.send('{"type":"subscribe","symbol":"AAPL"}') #Apple
    ws.send('{"type":"subscribe","symbol":"BINANCE:BTCUSDT"}') #Bitcoin
    ws.send('{"type":"subscribe","symbol":"BINANCE:ETHUSDT"}') #Ethereum

# This is the main function
if __name__ == "__main__":
    # enables the debug output
    websocket.enableTrace(True)
    # websocket connection with previously defined functions as parameters
    ws = websocket.WebSocketApp("wss://ws.finnhub.io?token=cqe0rvpr01qgmug3d06gcqe0rvpr01qgmug3d070",
                              on_message = on_message,
                              on_error = on_error,
                              on_close = on_close)
    # open the websocket
    ws.on_open = on_open
    # run the websocket forever
    ws.run_forever()

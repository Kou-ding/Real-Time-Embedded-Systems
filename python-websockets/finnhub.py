#https://pypi.org/project/websocket_client/
import websocket

def on_message(ws, message):
    print(message)

def on_error(ws, error):
    print(error)
    
def on_close(ws, close_status_code, close_msg):
    print("### closed ###")
    print("Close status code:", close_status_code)
    print("Close message:", close_msg)

def on_open(ws):
    #ws.send('{"type":"subscribe","symbol":"AMZN"}')
    #ws.send('{"type":"subscribe","symbol":"BINANCE:BTCUSDT"}')
    #ws.send('{"type":"subscribe","symbol":"IC MARKETS:1"}')
    ws.send('{"type":"subscribe","symbol":"TM"}') #Toyota
    ws.send('{"type":"subscribe","symbol":"NVDA"}') #Nvidia
    ws.send('{"type":"subscribe","symbol":"AMD"}') #AMD
    ws.send('{"type":"subscribe","symbol":"GOOGL"}') #Google
    ws.send('{"type":"subscribe","symbol":"AAPL"}') #Apple
    ws.send('{"type":"subscribe","symbol":"BINANCE:BTCUSDT"}') #Bitcoin
    ws.send('{"type":"subscribe","symbol":"BINANCE:ETHUSDT"}') #Ethereum
if __name__ == "__main__":
    websocket.enableTrace(True)
    ws = websocket.WebSocketApp("wss://ws.finnhub.io?token=cqe0rvpr01qgmug3d06gcqe0rvpr01qgmug3d070",
                              on_message = on_message,
                              on_error = on_error,
                              on_close = on_close)
    ws.on_open = on_open
    ws.run_forever()

# Import WebSocket client library (and others)
import websocket
import _thread
import time
import json
import gameModel

def unium_endpoint():
  return "http://localhost:8342"

def unium_socket():
  return "ws://localhost:8342/ws"



class ContextObj():
    def __init__(self):
        pass



# Define WebSocket callback functions
def ws_message(ws, message):
    print("WebSocket thread: %s" % message)

def ws_open(ws):
    print("Opened A!")
    print("dumping msg")
    msg = json.dumps( { "q": "/about" } )
    print(f"sending {msg}")    
    ws.send( msg )
    print(f"sent")

    #response = json.loads( await ws.recv() )

    #ws.send('{"event":"subscribe", "subscription":{"name":"trade"}, "pair":["XBT/USD","XRP/USD"]}')

USE_PLAIN_SOCKET = False
ws = None
gameModelInst = None

def ws_thread(*args):
    global ws
    ws = websocket.WebSocketApp(unium_socket(), on_open = ws_open, on_message = ws_message)
    ws.run_forever()
    print("Finished !!!")

if USE_PLAIN_SOCKET:
    # Start a new thread for the WebSocket interface
    threadId = _thread.start_new_thread(ws_thread, ())
else:
    contextInst = ContextObj()

    gameModelInst = gameModel.GameModel()
    gameModelInst.connectToGame()
    gameModelInst.load_subLevel("CaptureCoinsGood")
    gameModelInst.do_pickupTest(contextInst)


# Continue other (non WebSocket) tasks in the main thread
while True:
    time.sleep(5)
    print("Main thread: %d" % time.time())
    #print("closing socket")
    if USE_PLAIN_SOCKET:
        ws.close()
        print(ws)
    else:
        pass
        #gameModelInst.disconnectFromGame()

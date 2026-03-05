from pythonosc.dispatcher import Dispatcher
from pythonosc.osc_server import BlockingOSCUDPServer

def handler(address, *args):
    print(f"{address}: {args}")

dispatcher = Dispatcher()
dispatcher.map("/*", handler)

server = BlockingOSCUDPServer(("0.0.0.0", 8000), dispatcher)
print("Listening for OSC on port 8000...")
server.serve_forever()

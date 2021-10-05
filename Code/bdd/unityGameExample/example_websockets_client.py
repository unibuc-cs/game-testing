#!/usr/bin/env python

# WS client example

import asyncio
import websockets

async def hello():
    uri = "ws://localhost:8765"
    websocket  = await websockets.connect(uri)
    name = input("What's your name? ")

    await websocket.send(name)
    print(f">>> {name}")

    try:
        greeting = await asyncio.wait_for(websocket.recv(),timeout=5)
        print(f"<<< {greeting}")
    except asyncio.exceptions.TimeoutError:
        print("Exception timeout")


asyncio.run(hello())

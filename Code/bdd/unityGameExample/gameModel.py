import websockets
import asyncio
import logging
#logging.basicConfig(level=logging.DEBUG)
import unium
from hamcrest import assert_that, equal_to

import time
import json

def game_endpoint():
  return "http://localhost:8342"

def game_socket():
  return "ws://localhost:8342/ws"



class ContextObj():
  def __init__(self):
    pass
          

class GameModel():
  def __init__(self):
    self.socketOpened = None
    self.numRemainingPickups = None
    self.ws = None # The websocket underlying


  async def checkConnectionWebSocket(self, debugStatus):
    if self.ws is None or self.ws.open is False:
      self.ws =  await websockets.connect(game_socket(), ping_interval=None, close_timeout=None)    
      #print(f"Created websocket {self.ws}")
      self.socketOpened = unium.WebsocketHelper( self.ws )

    if debugStatus:
        # check to get the about
        about = await self.socketOpened.get( "/about" )
        assert about[ "Product" ] == "unium"
        #print(about)
        

  # Basic connection handlers to communicate with the game through websockets
  #---------------------------------------------------------
  async def connectToGame(self):
    #print("connecting to game")
    await self.checkConnectionWebSocket(debugStatus=False)
      
  def disconnectFromGame(self):
      self.ws.close()


  #---------------------------------------------------------

  # Some example of test functionality 
  #-------------------------------------------------------


  #def do_pickupTest(self, context):
  #    asyncio.run(self.testPickup(context), debugStatus=True)

  def getNumPickupsRemaining(self):
    return self.numRemainingPickups

  async def load_subLevel(self, subLevelName):
    await self.checkConnectionWebSocket(debugStatus=False)
    self.subLevelName = subLevelName

    # (re)load the tutorial scene (ensures it's in the start state)
    #print("Trying to reload scene")
    res = await self.socketOpened.get( "/utils/scene/" + subLevelName )
    #print("Scene loaded")
    assert res[ "scene" ] == subLevelName


  # Try to pickup all coins in the scene 
  async def testPickup(self, context):
    #print(f"In testPickup, self is {self}")
    await self.checkConnectionWebSocket(debugStatus=False)

    # get positions of all pickups
    pickups = await self.socketOpened.get( "/q/scene/Game/Pickup.transform.position" )

    # we should have 4 to start with
    assert len( pickups ) == 4

    # register for OnPickupCollected events
    await self.socketOpened.bind( "/scene/Game.Tutorial.OnPickupCollected" )

    startTime = time.time()

    # collect the pickups
    for pos in pickups:
      try:
        elapsed_time = time.time() - startTime
        remaining_time = context.timeLimit - elapsed_time

        await self.socketOpened.get( f"/q/scene/Game/Player.Player.MoveTo({pos})" )
        #print("Waiting for pickup...")
        #msg = json.loads(  await asyncio.wait_for(self.wait_rcv_from_socket(), timeout=5))
        await self.socketOpened.wait_for( "OnPickupCollected", timeout=remaining_time )
      except asyncio.exceptions.TimeoutError:
        print("Timeout !!")
        #context.failed = True
        return

    # store the number of pickups left in the level
    pickups = await self.socketOpened.get( "/q/scene/Game/Pickup.name" )
    self.numRemainingPickups = len( pickups )

    #print(context)

  #-------------------------------------------------------

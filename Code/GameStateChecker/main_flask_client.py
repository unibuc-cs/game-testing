from flask import Flask, request, Response
import requests
from typing import Dict, List
import jsonpickle
import json
import numpy as np
import cv2
import LogicLayer

# TODO: parametrize these
addr = 'http://localhost:5000'
check_visuals_url = addr + '/check_visuals'
check_sounds_url = addr + '/check_sounds '


# this is the public function to be imported and used by the game
class GameStateChecker_Client:
    def __init__(self):
        pass

    # TODO: use bytes not path on disk, i was too lazy to read color buffer correctly from Unreal Engine in our demo...
    @staticmethod
    def check_visuals_onScreenshot(screenShotPath,
                                   testContext : Dict[any, any],
                                   expectedAnswer : Dict[any, any]):
        # prepare headers for http request
        content_type = 'json'  # 'image/png'
        headers = {'content-type': content_type}

        # Read the img and send it as a bytestream later...
        img = cv2.imread(screenShotPath)
        # encode image as jpeg
        _, img_encoded = cv2.imencode('.png', img)

        # TODO: fix this to get bytes instead
        testContext["screenshotsCount"] = 1
        testContext["screenshotData"] = img_encoded.tostring()

        # send http request with image, some query info and receive response
        msg = {'testContext': testContext, 'expectedAnswer' : expectedAnswer}

        #print("Sending keys ", msg.keys())

        # encode response using jsonpickle
        msg_pickled = jsonpickle.encode(msg)

        # Get response
        response = requests.post(check_visuals_url, data=msg_pickled, headers=headers)

        # Decode response
        #print(json.loads(response.text))
        return response.text


# ============== FOR UNIT TEST PURPOSES ONLY ======================
if __name__=="__main__":
    checker_client = GameStateChecker_Client()

    # Test 1 - check weapon cross presence
    context = {
                # base parameters
                "requestCategory" : "UI",
                "requestFunc" : "checkWeaponCross",
                "screenshotsCount": 1,
                "screenshotData": None # to be filled later in the caller,

                # specific test parameters
                # none for this
                }

    res = checker_client.check_visuals_onScreenshot(screenShotPath = "unitTestResources/p1.png",
                                                    testContext=context,
                                                    expectedAnswer= { "boolResult" : "True" })
    
    print("Test weapon cross presence: ", res)
    #
    #
    #     #------------------------------------------
    #     # Test 2 - check ammo text in sync with the game information ?
    context = {
                # base parameters
                "requestCategory" : "UI",
                "requestFunc" : "checkAmmoSyncText",
                "screenshotsCount": 1,
                "screenshotData": None, # to be filled later in the caller,

                # specific test parameters
                "bbox": [912, 1015, 79, 49],  # Where should the text appear ?
                "textColorValueInHSV_min": 129,  # What color range are we expecting for text to have in the HSV space ?
                "textColorValueInHSV_max": 130,
                }
    expectedAnswer = {"intResult": 50} # How much should the ammo be on the screen ?

    res = checker_client.check_visuals_onScreenshot(screenShotPath = "unitTestResources/p1.png",
                                                testContext=context,
                                                expectedAnswer = expectedAnswer
                                              )

    print("Test ammo sync test: ", res)


# Here will come different services based on BaseUtils that can do things like:
# - understanding text from an image at a given coordinate
# - checking the pose of characters
# - etc...see the details in the documentation folder

import os
import time
from typing import List, Tuple, Dict
from VisionUtils import VisionUtils
import cv2

class LogicLayer:
    def __init__(self):
        self.preloadData()
        pass

    def __convertScreenshotToGray(self, screenshot):
        if len(screenshot.shape) > 2: # BGR ?
            screenshot_gray = cv2.cvtColor(screenshot, cv2.COLOR_BGR2GRAY)
        else:
            screenshot_gray = screenshot
        return screenshot_gray

    # Preload data that we would need...
    # Some refactoring like category grouping by resource type / test will be needed...
    def preloadData(self):
        self.templateImg_weaponCross = cv2.imread("unitTestResources/Cross_p.png", cv2.IMREAD_GRAYSCALE)

    # TODO: preload the template images !
    def testWeaponCrossPresence(self, screenshots : List[any],
                                context : Dict[any, any],
                                expectedAnswer : Dict[any, any],
                                debugEnabled=False):

        screenshot_gray = self.__convertScreenshotToGray(screenshots[0])
        isMatching = VisionUtils.matchTemplateImg(img_target=screenshot_gray,
                                                  img_src=self.templateImg_weaponCross

                                                  ,
                                                  minKeypoints=6,
                                                  debugEnabled=debugEnabled)
        return expectedAnswer["boolResult"] == str(isMatching)

    def testAmmoTextInSync(self,screenshots : List[any],
                              context : Dict[any, any],
                              expectedAnswer : Dict[any, any],
                              debugEnabled=False):

        #screenshot_gray = self.__convertScreenshotToGray(screenshots[0])
        res = VisionUtils.readTextFromPicture(srcImg=screenshots[0],
                                              boundingBox = context["bbox"],
                                              textValueColor_inHSV_min = context["textColorValueInHSV_min"],
                                              textValueColor_inHSV_max = context["textColorValueInHSV_max"],
                                                do_imgProcessing=True,
                                                debugEnabled=False)
        return expectedAnswer["intResult"] == int(res)



##### FOR UNIT TESTING PURPOSE ONLY
if __name__ == "__main__":
    #VisionUtils.preprocessimg_demo()

    logicLayer = LogicLayer()

    # Test 1
    unitTestScreenShot = cv2.imread("unitTestResources/p1.png", cv2.IMREAD_GRAYSCALE)
    res = logicLayer.testWeaponCrossPresence(screenshots=[unitTestScreenShot],
                                             context={},
                                             expectedAnswer= {"boolResult" : "True"},
                                             debugEnabled=False)
    print("testAmmoCrossPresence result is: " , res)

    # Test 2
    unitTestScreenShot = cv2.imread("unitTestResources/p1.png")#, cv2.IMREAD_BGR)
    res = logicLayer.testAmmoTextInSync(screenshots=[unitTestScreenShot],
                                             context={"bbox": [912, 1015, 79, 49],
                                                      "textColorValueInHSV_min": 129,
                                                      "textColorValueInHSV_max": 130},
                                             expectedAnswer= {"intResult" : 50},
                                             debugEnabled=False)
    print("unitTestScreenShot result is: " , res)


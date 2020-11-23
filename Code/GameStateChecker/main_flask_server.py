from flask import Flask, request, Response
import jsonpickle
import numpy as np
import cv2
import LogicLayer

# Initialize the Flask application
app = Flask(__name__)

logicLayer_inst = LogicLayer.LogicLayer()


# route http posts to this method
@app.route('/check_visuals', methods=['POST'])
def check_visuals():
    # Get the message
    r = request

    msg_received = jsonpickle.decode(r.data)
    # print(msg_received)
    assert (isinstance(msg_received, dict))

    print("Received keys ", msg_received.keys())

    testContext = msg_received["testContext"]
    requestCategory = testContext["requestCategory"]
    requestFunc = testContext["requestFunc"]
    requestExpectedAnswer = msg_received["expectedAnswer"]

    if requestCategory == "UI":
        res = testUI(context = testContext,
                     requestFunc = requestFunc,
                     requestExpectedAnswer = requestExpectedAnswer)

        # encode response using jsonpickle
        response_pickled = jsonpickle.encode(res)
        return Response(response=response_pickled, status=200, mimetype="application/json")
    else:
        raise  NotImplementedError("Not implemented")

# Test the results
def testUI(context, requestFunc, requestExpectedAnswer):
    # Step 1: decode the screenshots from context
    #--------------------------------------------------
    numScreenshots = context["screenshotsCount"]
    screenshotData = context["screenshotData"]
    # TODO: fix multiple screenshots sending
    # convert string of image data to uint8
    img_asnumpy = np.fromstring(screenshotData, np.uint8)
    # decode image
    imgs = [cv2.imdecode(img_asnumpy, cv2.IMREAD_COLOR)]

    # Step 2: check the type of test and conduct the appropiate check, get results
    res = 0
    if requestFunc == "checkWeaponCross":
        res = logicLayer_inst.testWeaponCrossPresence(imgs, context, requestExpectedAnswer)
    elif requestFunc == "checkAmmoSyncText":
        res = logicLayer_inst.testAmmoTextInSync(imgs, context, requestExpectedAnswer)
        pass
    else:
        raise NotImplementedError("Not implemented")

    # Pack the result and respond
    response = {'result': res}
    return response


# TODO add all other functionalities
@app.route('/check_sounds', methods=['POST'])
def check_sounds():
    pass

# start flask app
app.run(host="0.0.0.0", port=5000)


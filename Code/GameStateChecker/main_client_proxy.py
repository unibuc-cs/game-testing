from flask import Flask, request, Response, jsonify
import jsonpickle
import numpy as np
import cv2
import LogicLayer
import json

import main_flask_client as adapterClient

# Initialize the Flask application
app = Flask(__name__)

logicLayer_inst = LogicLayer.LogicLayer()

@app.route('/check_visuals_proxy', methods=['GET', 'POST'])
def check_visuals_proxy():
    data = request.data
    formData = request.form
    frameCount = int(formData["frameCount"])
    imgBytes = request.files["fileUpload"].read()
    annotations_jsonStr = formData["annotations"]

    annotations_deserialized = json.loads(annotations_jsonStr)

    # request.form to get form parameter
    filePath = f"screenShot_{frameCount}.png"
    outImg = open(filePath, "wb")
    outImg.write(imgBytes)


    img_numpy = np.fromstring(imgBytes, dtype="uint8")
    img_decoded = cv2.imdecode(img_numpy, cv2.IMREAD_UNCHANGED)
    imgRes = img_decoded.shape


    for entityName, entityRectDict in annotations_deserialized.items():
        bboxWidth = int(entityRectDict['width'])
        bboxHeight = int(entityRectDict['height'])
        bboxCenter = (int(entityRectDict['x']), imgRes[0] - int(entityRectDict['y']))
        #bboxMin = (int(bboxCenter[0] - bboxWidth/2), int(bboxCenter[1] - bboxHeight/2))
        #bboxMax = (int(bboxCenter[0] + bboxWidth/2), int(bboxCenter[1] + bboxHeight/2))
        bboxMin = (int(bboxCenter[0]), int(bboxCenter[1] - bboxHeight))
        bboxMax = (int(bboxCenter[0] + bboxWidth), int(bboxCenter[1]))

        color = (0, 0, 255)
        thickness = 5
        img_decoded = cv2.rectangle(img_decoded, bboxMin, bboxMax, color, thickness)

    img_decoded = np.array(img_decoded)
    outAugImg = cv2.imwrite("aug_"+filePath, img_decoded)

    #adapterClient.GameStateChecker_Client.check_visuals_onScreenshot
    return jsonify(200)

    """ # Transforms to a dictionary 
    return jsonify(username=g.user.username,
                   email=g.user.email,
                   id=g.user.id)
    """

# start flask app
app.run(host="0.0.0.0", port=5001)

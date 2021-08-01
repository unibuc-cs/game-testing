from flask import Flask, request, Response, jsonify
import jsonpickle
import numpy as np
import cv2
import LogicLayer

import main_flask_client as adapterClient

# Initialize the Flask application
app = Flask(__name__)

logicLayer_inst = LogicLayer.LogicLayer()

@app.route('/check_visuals_proxy', methods=['GET', 'POST'])
def check_visuals_proxy():
    data = request.data
    formData = request.form
    frameCount = int(formData["frameCount"])
    imgUploaded = request.files["fileUpload"].read()

    # request.form to get form parameter
    outImg = open(f"screenShot_{frameCount}.png", "wb")
    outImg.write(imgUploaded)

    #adapterClient.GameStateChecker_Client.check_visuals_onScreenshot
    return jsonify(200)

    """ # Transforms to a dictionary 
    return jsonify(username=g.user.username,
                   email=g.user.email,
                   id=g.user.id)
    """

# start flask app
app.run(host="0.0.0.0", port=5001)

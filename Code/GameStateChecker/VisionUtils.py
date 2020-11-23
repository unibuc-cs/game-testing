# TODO: copy the foundation things from the other files...
# These will be the entry for tensorflow, open3d pose, opencv, nlp toolkits and so on..
import cv2
import os
import pytesseract
from PIL import Image
import time
import numpy as np
from typing import List, Tuple

from matplotlib import pyplot as plt


class VisionUtils:
    # low-level services here as static
    def __init__(self):
        pass

    # Returns the text at a given source image, specified bounding box, V space for text in HSV space, and if it should do img processing or not
    # bbox in x,y, width, height
    @staticmethod
    def readTextFromPicture(srcImg,
                            boundingBox: Tuple[float, float, float, float],
                            textValueColor_inHSV_min, textValueColor_inHSV_max,
                            do_imgProcessing,
                            debugEnabled=False):

        if isinstance(srcImg, str):
            img = cv2.imread(srcImg, cv2.COLOR_BGR2RGB)
        else:
            img = cv2.cvtColor(srcImg, cv2.COLOR_BGR2RGB)

        img = img[boundingBox[1]:boundingBox[1] + boundingBox[3], boundingBox[0]:boundingBox[0] + boundingBox[2], :]
        processed_img = img

        if do_imgProcessing:
            hsv = cv2.cvtColor(img, cv2.COLOR_BGR2HSV)

            if debugEnabled:
                cv2.imshow("original image", processed_img)
                cv2.waitKey()
                cv2.imshow("hsv", hsv)
                cv2.waitKey()

            # Process the hsv image space using the colors knowing for drawing the text
            mask = np.logical_and(hsv[:, :, 2] >= textValueColor_inHSV_min, hsv[:, :, 2] <= textValueColor_inHSV_max)
            # print(np.count_nonzero(mask))
            # print(mask.shape)
            # print(mask)
            hsv[mask] = 255
            hsv[~mask] = 0

            # Average filtering, Sharpen and morpohologically closing operations
            hsv = hsv[:, :, 2]  # keep only value channel => gray space
            # cv2.imshow("res", hsv)
            kernel = np.ones((5, 5), np.float32) / 25
            dst = cv2.filter2D(hsv, -1, kernel)
            thresh = 255 - cv2.threshold(dst, 0, 255, cv2.THRESH_BINARY_INV + cv2.THRESH_OTSU)[1]

            kernel = cv2.getStructuringElement(cv2.MORPH_RECT, (3, 3))
            close = cv2.morphologyEx(thresh, cv2.MORPH_CLOSE, kernel, iterations=1)

            # Erode a bit
            kernel = np.ones((2, 2), np.uint8)
            erosion = cv2.erode(close, kernel, iterations=1)

            processed_img = erosion

        text = pytesseract.image_to_string(processed_img)
        if debugEnabled:
            print(text)
            cv2.imshow("processed image", processed_img)
            cv2.waitKey()

        return text

    # Just a dummy template matching function between a source image and a template, classic opencv methods, no NN here.
    # Searches for a template img in a src Img
    # options: distance threshold to consider for features, number of keypoints
    @staticmethod
    def matchTemplateImg(img_target, img_src, minKeypoints=6, distThreshold=None, debugEnabled=False):
        if debugEnabled:
            cv2.imshow("src", img_src)
            cv2.imshow("target", img_target)

        useORB = False
        useSIFT = True

        if useORB:
            # Initiate ORB detector
            orb = cv2.ORB_create()
            # find the keypoints and descriptors with ORB
            kp1, des1 = orb.detectAndCompute(img_src, None)
            kp2, des2 = orb.detectAndCompute(img_target, None)

            # create BFMatcher object
            bf = cv2.BFMatcher(cv2.NORM_HAMMING, crossCheck=True)
            # Match descriptors.
            matches = bf.match(des1, des2)
            # Sort them in the order of their distance.
            matches = sorted(matches, key=lambda x: x.distance)
            # Draw first 10 matches.
            img3 = cv2.drawMatches(img_src, kp1, img_target, kp2, matches[:10], None, flags=cv2.DrawMatchesFlags_NOT_DRAW_SINGLE_POINTS)

            if debugEnabled:
                cv2.imshow("matches", img3)
                cv2.waitKey()

            return len(matches) > minKeypoints
        elif useSIFT:
            sift = cv2.xfeatures2d.SIFT_create()
            # find the keypoints and descriptors with SIFT
            kp1, des1 = sift.detectAndCompute(img_src, None)
            kp2, des2 = sift.detectAndCompute(img_target, None)
            # BFMatcher with default params
            bf = cv2.BFMatcher()
            matches = bf.knnMatch(des1, des2, k=2)
            if False:
                # Apply ratio test
                good = []
                for m, n in matches:
                    if m.distance < 0.75 * n.distance:
                        good.append([m])
                # cv.drawMatchesKnn expects list of lists as matches.
                img3 = cv2.drawMatchesKnn(img_src, kp1, img_target, kp2, matches, None, flags=cv2.DrawMatchesFlags_NOT_DRAW_SINGLE_POINTS)
            else:
                img3 = cv2.drawMatchesKnn(img_src, kp1, img_target, kp2, matches, None, flags=cv2.DrawMatchesFlags_NOT_DRAW_SINGLE_POINTS)

            if debugEnabled:
                cv2.imshow("matches", img3)
                cv2.waitKey()

            return len(matches) > minKeypoints

    @staticmethod
    def preprocessimg_demo():
        img = cv2.imread("Cross.png", cv2.COLOR_BGR2GRAY)
        img[img < 150] = 0
        cv2.imshow("processed image", img)
        cv2.waitKey()
        cv2.imwrite("Cross_p.png", img)
        exit(0)

import cv2
import numpy as np
import copy
import math
import struct
import sys
from scipy import signal
import tkinter as Tk
#from appscript import app
import audiopanel as ap
import pyaudio
from allfunctions import *


# parameters
cap_region_x_begin=0.5  # start point/total width
cap_region_y_end=0.8  # start point/total width
threshold = 60  #  BINARY threshold
blurValue = 41  # GaussianBlur parameter
bgSubThreshold = 50
learning_rate = 0
BLOCKLEN = 256
freq = 0
gain_step = 0

def sub_background(frame, lr):
    foreground_mask = bgModel.apply(frame,learningRate=lr)
    # kernel = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (3, 3))
    # res = cv2.morphologyEx(foreground_mask, cv2.MORPH_OPEN, kernel)

    kernel = np.ones((3, 3), np.uint8)
    foreground_mask = cv2.erode(foreground_mask, kernel, iterations=1)
    res = cv2.bitwise_and(frame, frame, mask=foreground_mask)
    return res


def n_fingers(res,drawing):  # -> finished bool, cnt: finger count
    #  convexity defect
    hull = cv2.convexHull(res, returnPoints=False)
    if len(hull) > 3:
        defects = cv2.convexityDefects(res, hull)
        if type(defects) != type(None):  # avoid crashing.   (BUG not found)

            cnt = 0
            for i in range(defects.shape[0]):  # calculate the angle
                s, e, f, d = defects[i][0]
                start = tuple(res[s][0])
                end = tuple(res[e][0])
                far = tuple(res[f][0])
                a = math.sqrt((end[0] - start[0]) ** 2 + (end[1] - start[1]) ** 2)
                b = math.sqrt((far[0] - start[0]) ** 2 + (far[1] - start[1]) ** 2)
                c = math.sqrt((end[0] - far[0]) ** 2 + (end[1] - far[1]) ** 2)
                angle = math.acos((b ** 2 + c ** 2 - a ** 2) / (2 * b * c))  # cosine theorem
                if angle <= math.pi / 2:  # angle less than 90 degree, treat as fingers
                    cnt += 1
                    cv2.circle(drawing, far, 8, [211, 84, 0], -1)
            return True, cnt
    return False, 0
# variables
isBgCaptured = 0   # bool, whether the background captured
keypress_check = False  # if true, keyborad simulator works

# Camera
camera = cv2.VideoCapture(0)
camera.set(10,200)
cv2.namedWindow('Set threshold')
cv2.createTrackbar('Threshold', 'Set threshold', threshold, 100, display_thresh)

st = "{} is selected".format(0)

p = pyaudio.PyAudio()
stream = p.open(
  format = pyaudio.paInt16,  
  channels = 1, 
  rate = 16000,
  input = True, 
  output = True,
  frames_per_buffer = 128)

output_block = np.zeros(BLOCKLEN, dtype = int)
# output_block = [0 for n in range(0, BLOCKLEN)]
theta = np.linspace(0,0,BLOCKLEN)
gain = int(1000)
print(theta)


while camera.isOpened():
    ret, frame = camera.read()
    threshold = cv2.getTrackbarPos('Threshold', 'Set threshold')
    frame = cv2.bilateralFilter(frame, 5, 50, 100)  # smoothing filter
    frame = cv2.flip(frame, 1)  # flip the frame horizontally
    cv2.rectangle(frame, (int(cap_region_x_begin * frame.shape[1]), 0),
                 (frame.shape[1], int(cap_region_y_end * frame.shape[0])), (255, 0, 0), 2)
    cv2.imshow('Live Feed', frame)

    #  Main operation
    if isBgCaptured == 1:  # this part wont run until background captured
        img = sub_background(frame, learning_rate)
        img = img[0:int(cap_region_y_end * frame.shape[0]),
                    int(cap_region_x_begin * frame.shape[1]):frame.shape[1]]  # clip the ROI
        cv2.imshow('Background Mask', img)

        # convert the image into binary image
        gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
        blur = cv2.GaussianBlur(gray, (blurValue, blurValue), 0)
        cv2.imshow('Gaussian Blur', blur)
        ret, thresh = cv2.threshold(blur, threshold, 255, cv2.THRESH_BINARY)
        cv2.imshow('Thresholding', thresh)


        # get the coutours
        thresh1 = copy.deepcopy(thresh)
        _,contours, hierarchy = cv2.findContours(thresh1, cv2.RETR_TREE, cv2.CHAIN_APPROX_SIMPLE)
        length = len(contours)
        maxArea = -1
        if length > 0:
            for i in range(length):  # find the biggest contour (according to area)
                temp = contours[i]
                area = cv2.contourArea(temp)
                if area > maxArea:
                    maxArea = area
                    ci = i

            res = contours[ci]
            hull = cv2.convexHull(res)
            drawing = np.zeros(img.shape, np.uint8)
            # string_to_out = "%d".format(cnt)
            cv2.drawContours(drawing, [res], 0, (0, 255, 0), 2)
            cv2.drawContours(drawing, [hull], 0, (0, 0, 255), 3)

            isFinishCal,cnt = n_fingers(res,drawing)
            if keypress_check is True:
                if isFinishCal is True and cnt <= 2:
                    print (cnt)
                    #app('System Events').keystroke(' ')  # simulate pressing blank space
            cv2.putText(drawing, st, (20,20), cv2.FONT_HERSHEY_SIMPLEX,0.9,(255,0,0),2)    
            if cnt == 1:
                st = "Updating frequency"
                freq += 0.2*math.pi
                theta = update_theta(freq,cnt)
                # print(theta)
                output_block = np.int16(np.multiply(gain,np.cos(theta)))
                output_string = struct.pack('h'*BLOCKLEN, *output_block)
                stream.write(output_string)
            elif cnt == 2:
                # st = "Updating gain"
                gain = update_gain(gain, cnt)
                st = "Updating gain {}".format(gain)
                # print(gain)
                output_block = np.int16(np.multiply(gain,np.cos(theta)))
                output_string = struct.pack('h'*BLOCKLEN, *output_block)
                stream.write(output_string)
            if cnt == 3:
                st = "Updating frequency"
                freq -= 0.2*math.pi
                if freq == 0:
                    st = "Zero frequency"
                theta = update_theta(freq,cnt)
                # print(theta)
                output_block = np.int16(np.multiply(gain,np.cos(theta)))
                output_string = struct.pack('h'*BLOCKLEN, *output_block)
                stream.write(output_string)
            elif cnt == 4:
                
                gain = update_gain(gain, cnt)
                st = "Updating gain {}".format(gain)
                # print(gain)
                output_block = np.int16(np.multiply(gain,np.cos(theta)))
                output_string = struct.pack('h'*BLOCKLEN, *output_block)
                stream.write(output_string)
            elif cnt > 4:
                output_bl = stream.read(BLOCKLEN, exception_on_overflow=False)
                output_var = struct.unpack('h'*BLOCKLEN, output_bl)
                output_var = np.int16(np.multiply(output_var,gain))
                output_string = struct.pack('h'*BLOCKLEN, *output_var)
                st = "Mic input"
                stream.write(output_string)
        cv2.imshow('output', drawing)


    # Keyboard OP
    k = cv2.waitKey(10)
    if k == 27:  # press ESC to exit
        break
    elif k == ord('b'):  # press 'b' to capture the background
        bgModel = cv2.createBackgroundSubtractorMOG2(0, bgSubThreshold)
        isBgCaptured = 1
        print( '!!!Background Captured!!!')
    elif k == ord('r'):  # press 'r' to reset the background
        bgModel = None
        keypress_check = False
        isBgCaptured = 0
        print ('!!!Reset BackGround!!!')
    elif k == ord('n'):
        keypress_check = True
        print ('!!!Trigger On!!!')
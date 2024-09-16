#!/usr/bin/env python

######## Number Classifer for Meter Reading Using Tensorflow-trained Classifier #########
#
# Author: Cherntay Shih
# Date: 04/16/21
# Description: 
# This program uses a Tensorflow Lite model primarily for classifying MNIST dataset
# to classify numbers of meter. It import the picture from Google Drive and crop the image to run the
# Tensorflow model to classify the numbers then stitch data to send to InfluxDB to visualize on 
# Grafana.
# License under Apache 2.0

# Import packages
import io
import sys
import base64
import json

import cv2
import numpy as np
import tensorflow as tf

from tflite_runtime.interpreter import Interpreter

def code():
    # Directory path
    directory_path = "/home/pi/Documents/"
    model_directory_path = directory_path + "MNIST_Training.tflite"

    # Load the TFLite model and allocate memories for tensors
    interpreter = Interpreter(model_path = model_directory_path)
    interpreter.allocate_tensors()

    # Get input and output tensors
    input_details = interpreter.get_input_details()
    output_details = interpreter.get_output_details()

    # Parse the msg.payload from nodered base64 to script
    base64_string = sys.argv[2]

    # Construct image as numpy array
    decode = base64.b64decode(base64_string)
    im_arr = np.frombuffer(decode, dtype=np.uint8)  # im_arr is one-dim Numpy array
    image = cv2.imdecode(im_arr, flags=cv2.IMREAD_COLOR)
    image = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)

    # Cropping Images
    first_cropped_image = image[100:125, 160:175]
    second_cropped_image = image[100:125, 175:190]
    third_cropped_image = image[100:125, 190:205]
    fourth_cropped_image = image[100:125, 205:220]
    fifth_cropped_image = image[100:125, 220:235]
    sixth_cropped_image = image[100:125, 235:250]

    # Gaussian Blur
    first_blur_image = cv2.GaussianBlur(first_cropped_image, (1,1), 0)
    second_blur_image = cv2.GaussianBlur(second_cropped_image, (1,1), 0)
    third_blur_image = cv2.GaussianBlur(third_cropped_image, (1,1), 0)
    fourth_blur_image = cv2.GaussianBlur(fourth_cropped_image, (1,1), 0)
    fifth_blur_image = cv2.GaussianBlur(fifth_cropped_image, (1,1), 0)
    sixth_blur_image = cv2.GaussianBlur(sixth_cropped_image, (1,1), 0)

    # Binary Threshold
    _, first_threshold = cv2.threshold(first_blur_image, 100,255, cv2.THRESH_BINARY)
    _, second_threshold = cv2.threshold(second_blur_image, 100,255, cv2.THRESH_BINARY)
    _, third_threshold = cv2.threshold(third_blur_image, 100,255, cv2.THRESH_BINARY)
    _, fourth_threshold = cv2.threshold(fourth_blur_image, 100,255, cv2.THRESH_BINARY)
    _, fifth_threshold = cv2.threshold(fifth_blur_image, 100,255, cv2.THRESH_BINARY)
    _, sixth_threshold = cv2.threshold(sixth_blur_image, 100,255, cv2.THRESH_BINARY)

    # Resize Images (28,28)
    first_resized_image = cv2.resize(first_threshold, (28,28), interpolation=cv2.INTER_AREA)
    second_resized_image = cv2.resize(second_threshold, (28,28), interpolation=cv2.INTER_AREA)
    third_resized_image = cv2.resize(third_threshold, (28,28), interpolation=cv2.INTER_AREA)
    fourth_resized_image = cv2.resize(fourth_threshold, (28,28), interpolation=cv2.INTER_AREA)
    fifth_resized_image = cv2.resize(fifth_threshold, (28,28), interpolation=cv2.INTER_AREA)
    sixth_resized_image = cv2.resize(sixth_threshold, (28,28), interpolation=cv2.INTER_AREA)


    # Reshape images and include color depth
    first_image = first_resized_image.reshape(-1,28,28,1).astype(np.float32) / 255
    second_image = second_resized_image.reshape(-1,28,28,1).astype(np.float32) / 255
    third_image = third_resized_image.reshape(-1,28,28,1).astype(np.float32) / 255
    fourth_image = fourth_resized_image.reshape(-1,28,28,1).astype(np.float32) / 255
    fifth_image = fifth_resized_image.reshape(-1,28,28,1).astype(np.float32) / 255
    sixth_image = sixth_resized_image.reshape(-1,28,28,1).astype(np.float32) / 255
    
    # TFLite running inferences to classify digits
    input_details = interpreter.get_input_details()
    interpreter.set_tensor(input_details[0]['index'], first_image)    
    interpreter.invoke()
    output_details = interpreter.get_output_details()
    first_output = interpreter.get_tensor(output_details[0]['index'])
    first_digit = np.argmax(first_output)

    input_details = interpreter.get_input_details()
    interpreter.set_tensor(input_details[0]['index'], second_image)    
    interpreter.invoke()
    output_details = interpreter.get_output_details()
    second_output = interpreter.get_tensor(output_details[0]['index'])
    second_digit = np.argmax(second_output)

    input_details = interpreter.get_input_details()
    interpreter.set_tensor(input_details[0]['index'], third_image)    
    interpreter.invoke()
    output_details = interpreter.get_output_details()
    third_output = interpreter.get_tensor(output_details[0]['index'])
    third_digit = np.argmax(third_output)

    input_details = interpreter.get_input_details()
    interpreter.set_tensor(input_details[0]['index'], fourth_image)    
    interpreter.invoke()
    output_details = interpreter.get_output_details()
    fourth_output = interpreter.get_tensor(output_details[0]['index'])
    fourth_digit = np.argmax(fourth_output)

    input_details = interpreter.get_input_details()
    interpreter.set_tensor(input_details[0]['index'], fifth_image)    
    interpreter.invoke()
    output_details = interpreter.get_output_details()
    fifth_output = interpreter.get_tensor(output_details[0]['index'])
    fifth_digit = np.argmax(fifth_output)

    input_details = interpreter.get_input_details()
    interpreter.set_tensor(input_details[0]['index'], sixth_image)    
    interpreter.invoke()
    output_details = interpreter.get_output_details()
    sixth_output = interpreter.get_tensor(output_details[0]['index'])
    sixth_digit = np.argmax(sixth_output)

    # Concatenation integer numbers
    stitched_integer_numbers = int(str(first_digit) + str(second_digit) + str(third_digit) + str(fourth_digit) + str(fifth_digit) + str(sixth_digit)) / 100

    JSON_file = {}
    JSON_file["meter"] = stitched_integer_numbers
    JSON_file = json.dumps(JSON_file)
    print(JSON_file)

if __name__ == "__main__":
    code()

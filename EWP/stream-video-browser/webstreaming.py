# USAGE
# python webstreaming.py --ip 0.0.0.0 --port 8000

# import the necessary packages
# grabbing frames from the camera
from imutils.video import VideoStream
# web server (builds the web page)
from flask import Response
from flask import Flask
from flask import render_template
# allows multiple clients to connect at the same time
import threading
# parse arguments
import argparse
# time / date
import datetime
import time as t
# rework / treat images
import imutils
# openCV
import cv2

# distance from the border for the text
OFFSET = 10

# temp width, adjust with smartphone / ... width
WIDTH = 800

# timestamp FORMATTING: dd/mm/yyyy, hh:mm:ss
FORMAT = "%d/%m/%Y, %H:%M:%S"

# general text stuff
# set font
font = cv2.FONT_HERSHEY_SIMPLEX
# set color
fontColor = (0, 0, 255) # red
# set line thickness
lineThickness = 1

# text stuff for the watermark
watermark = "Drone V2 ISFATES - Camera Embarquee"
# size of the text
fontScaleWatermark = 0.5
# get the width and height to position the text better
(tmpWidth, tmpHeight) = cv2.getTextSize(watermark, font, fontScaleWatermark, lineThickness)
# position for the watermark. Bottom left corner.
positionWatermark = (OFFSET, OFFSET + tmpHeight * 2)

# positioning stuff for the timestamp
# position for the timestamp. Bottom left corner
positionTimestamp = (OFFSET, (OFFSET + tmpHeight*2)*2)
# get the time
time = datetime.datetime.now()
# font size
fontScaleTimestamp = 0.4


# initialize the output frame and a lock used to ensure thread-safe
# exchanges of the output frames (useful for multiple browsers/tabs
# are viewing tthe stream)
outputFrame = None
lock = threading.Lock()

# initialize a flask object
app = Flask(__name__)

# initialize the video stream
vs = VideoStream(usePiCamera=1).start()
#vs = VideoStream(src=0).start()
# camera warmup time
t.sleep(2.0)


@app.route("/")
def index():
	# return the rendered template
	return render_template("index.html")

def process_frame():
	# needs to run in a loop, otherwise the app will only display one frame
	# updates the image on every run
	while True:
		# grab global references to the video stream, output frame, and
		# lock variables
		global vs, outputFrame, lock
		# get a frame from the cam
		frame = vs.read()
		#resize it using the width we want
		frame = imutils.resize(frame, width=WIDTH)
		# add the watermark to the image
		cv2.putText(frame, watermark, positionWatermark, font, fontScaleWatermark, fontColor, lineThickness)
		# get time
		time = datetime.datetime.now()
		# write time to the image
		cv2.putText(frame, time.strftime(FORMAT), positionTimestamp, font, fontScaleTimestamp, fontColor, lineThickness)
		# acquire the lock, set the output frame, and release the
		# lock
		with lock:
			# write the "finished" frame to the variable to be updated on the webpage
			outputFrame = frame.copy()
		
def generate():
	# grab global references to the output frame and lock variables
	global outputFrame, lock

	# loop over frames from the output stream
	while True:
		# wait until the lock is acquired
		with lock:
			# check if the output frame is available, otherwise skip
			# the iteration of the loop
			if outputFrame is None:
				continue

			# encode the frame in JPEG format
			(flag, encodedImage) = cv2.imencode(".jpg", outputFrame)

			# ensure the frame was successfully encoded
			if not flag:
				continue

		# yield the output frame in the byte format
		yield(b'--frame\r\n' b'Content-Type: image/jpeg\r\n\r\n' + 
			bytearray(encodedImage) + b'\r\n')

@app.route("/video_feed")
def video_feed():
	# return the response generated along with the specific media
	# type (mime type)
	return Response(generate(),
		mimetype = "multipart/x-mixed-replace; boundary=frame")

# check to see if this is the main thread of execution
if __name__ == '__main__':
	# construct the argument parser and parse command line arguments
	ap = argparse.ArgumentParser()
	ap.add_argument("-i", "--ip", type=str, required=True,
		help="ip address of the device")
	ap.add_argument("-o", "--port", type=int, required=True,
		help="ephemeral port number of the server (1024 to 65535)")
	args = vars(ap.parse_args())

	# start a thread that will process the frames
	t = threading.Thread(target=process_frame)
	t.daemon = True
	t.start()

	# start the flask app
	app.run(host=args["ip"], port=args["port"], debug=True,
		threaded=True, use_reloader=False)

# release the video stream pointer
vs.stop()
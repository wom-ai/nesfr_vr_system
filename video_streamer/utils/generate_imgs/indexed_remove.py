import os
import sys

def img_path(camera_number, i):
	return "video" + str(camera_number) + "/image" + str(i) + ".png"

for i in range(1, len(sys.argv)):
	index = int(sys.argv[i])
	os.remove(img_path(0, index))
	os.remove(img_path(2, index))


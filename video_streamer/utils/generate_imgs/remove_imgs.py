import os
import sys


start = int(sys.argv[1])
end = int(sys.argv[2])

def img_path(camera_number, i):
	return "video" + str(camera_number) + "/image" + str(i) + ".png"



# start = 25
# end = 100

for i in range(start, end):
	os.remove(img_path(0, i))
	os.remove(img_path(2, i))
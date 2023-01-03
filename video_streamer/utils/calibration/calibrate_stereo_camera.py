import numpy as np
import cv2 as cv
import os

import glob

def img_path(camera_number, i):
	return "video" + str(camera_number) + "/image" + str(i) + ".png"


imgs = glob.glob("video0/*")

number_of_images = 0

for img in imgs:
	number = int(img[12: img.find(".")])

	if number > number_of_images:
		number_of_images = number


number_of_images += 1





# clean = True

show_corners = True

# termination criteria
criteria = (cv.TERM_CRITERIA_EPS + cv.TERM_CRITERIA_MAX_ITER, 1000, 0.001)
# prepare object points, like (0,0,0), (1,0,0), (2,0,0) ....,(6,5,0)
objp = np.zeros((6*9,3), np.float32)
objp[:,:2] = np.mgrid[0:9,0:6].T.reshape(-1,2)
# Arrays to store object points and image points from all the images.
objpoints = [] # 3d point in real world space

imgpoints0 = [] # 2d points in image plane.
imgpoints2 = [] # 2d points in image plane.

processed = []
sorted_index = 0

for i in range(0, number_of_images):
	img0 = cv.imread(img_path(0, i))
	img2 = cv.imread(img_path(2, i))

	if img0 is None:
		continue

	gray0 = cv.cvtColor(img0, cv.COLOR_BGR2GRAY)
	gray2 = cv.cvtColor(img2, cv.COLOR_BGR2GRAY)


	ret0, corners0 = cv.findChessboardCorners(gray0, (9,6), None)
	ret2, corners2 = cv.findChessboardCorners(gray2, (9,6), None)


	if not ret0 and not ret2:
		print(str(i) + ": processed none") 

	if ret0 and not ret2:
		print(str(i) + ": processed 0 but not 2") 

	if not ret0 and ret2:
		print(str(i) + ": processed 2 but not 0") 

	if ret0 and ret2:
		objpoints.append(objp)

		drawn_corners0 = cv.cornerSubPix(gray0, corners0, (11,11), (-1,-1), criteria)
		drawn_corners2 = cv.cornerSubPix(gray2, corners2, (11,11), (-1,-1), criteria)

		imgpoints0.append(corners0)
		imgpoints2.append(corners2)

		if show_corners:
			cv.drawChessboardCorners(img0, (9,6), drawn_corners0, ret0)
			cv.drawChessboardCorners(img2, (9,6), drawn_corners2, ret2)

			cv.imshow('img0', img0)
			cv.imshow('img2', img2)

			cv.waitKey(0)

		print(str(i) + ": processed both") 

		processed.append(i)

		os.rename(img_path(0, i), img_path(0, sorted_index))
		os.rename(img_path(2, i), img_path(2, sorted_index))
		sorted_index += 1
	else:
		os.remove(img_path(0, i))
		os.remove(img_path(2, i))


print("number of processed images: " + str(len(processed)))
print("processed images: " + str(processed))



cv.destroyAllWindows()



# if clean:
# 	sorted_index = 0

# 	for i in range(number_of_images):
# 		if i not in processed:
# 			os.remove(img_path(0, i))
# 			os.remove(img_path(2, i))
# 		else:
# 			os.rename(img_path(0, i), img_path(0, sorted_index))
# 			os.rename(img_path(2, i), img_path(2, sorted_index))
# 			sorted_index += 1


# img_size = np.array([1920, 1080])


# img0 = cv.imread(img_path(0, i))
# gray0 = cv.cvtColor(img0, cv.COLOR_BGR2GRAY)

img_size = gray0.shape[::-1]



# init_camera_matrix = np.array([[577.9090654,    0,         998.36434283],
#  [  0,         576.02181904, 567.25797745],
#  [  0,           0,           1,        ]])

# init_dist = np.array([[-0.17698078,  0.02976428,  0.00205374,  0.00064214, -0.00214519]])

np.set_printoptions(suppress=True)


#last one


# init_camera_matrix = np.array([[ 741.5437805,     0,         1012.23934504],
#  [   0,          739.99734798,  590.60736904],
#  [   0,            0,            1        ]])

# init_dist = np.array([[-0.29082577,  0.07514147, -0.0022609,  -0.00132757, -0.00816015]])


init_camera_matrix0 = np.array([[649.98291315,   0,         948.25829987],
 [  0,         649.5200055,  562.09198792],
 [  0,           0,           1,        ]])

init_dist0 = np.array([[-0.21252438,  0.03947589,  0.00009263,  0.0000544,  -0.00312653]])


init_camera_matrix2 = np.array([[649.98291315,   0,         948.25829987],
 [  0,         649.5200055,  562.09198792],
 [  0,           0,           1,        ]])

init_dist2 = np.array([[-0.21252438,  0.03947589,  0.00009263,  0.0000544,  -0.00312653]])


# init_dist0 = np.array([[-0.358074811139381, 0.150366096279157, -0.000239617440106, -0.001364488806427, -0.031502910462795]])
# init_dist2 = np.array([[-0.358074811139381, 0.150366096279157, -0.000239617440106, -0.001364488806427, -0.031502910462795]])

# flags = cv.CALIB_USE_INTRINSIC_GUESS | cv.CALIB_SAME_FOCAL_LENGTH

# ret, camera_matrix0, dist0, camera_matrix2, dist2, R, T, E, F = cv.stereoCalibrate(objpoints, imgpoints0, imgpoints2, init_camera_matrix0, init_dist0, init_camera_matrix2, init_dist2, img_size, criteria = criteria, flags = flags)

# ret, camera_matrix0, dist0, camera_matrix2, dist2, R, T, E, F = cv.stereoCalibrate(objpoints, imgpoints0, imgpoints2, None, None, None, None, img_size, criteria = criteria, flags = 0)

flags = cv.CALIB_USE_INTRINSIC_GUESS

ret, camera_matrix0, dist0, rvecs, tvecs = cv.calibrateCamera(objpoints, imgpoints0, img_size, init_camera_matrix0, init_dist0, flags = flags, criteria = criteria)

ret, camera_matrix2, dist2, rvecs, tvecs = cv.calibrateCamera(objpoints, imgpoints2, img_size, init_camera_matrix2, init_dist2, flags = flags, criteria = criteria)

# ret, camera_matrix0, dist0, rvecs, tvecs = cv.calibrateCamera(objpoints, imgpoints0, img_size,  None, None)

# ret, camera_matrix2, dist2, rvecs, tvecs = cv.calibrateCamera(objpoints, imgpoints2, img_size,  None, None)

# camera_matrix0 = init_camera_matrix
# camera_matrix2 = init_camera_matrix

# dist0 = init_dist
# dist2 = init_dist



# print(R)
# print(T)
# print(E)
# print(F)


new_camera_matrix0, roi0 = cv.getOptimalNewCameraMatrix(camera_matrix0, dist0, img_size, 0, img_size)
new_camera_matrix2, roi2 = cv.getOptimalNewCameraMatrix(camera_matrix2, dist2, img_size, 0, img_size)

print(roi0)
print(roi2)

print("camera_matrix0")
print(repr(camera_matrix0))
print("camera_matrix2")
print(repr(camera_matrix2))

print("dist0")
print(repr(dist0))
print("dist2")
print(repr(dist2))


img = cv.imread(img_path(0, 0))

undistorted0 = cv.undistort(img, camera_matrix0, dist0, None, new_camera_matrix0)
undistorted2 = cv.undistort(img, camera_matrix2, dist2, None, new_camera_matrix2)


x, y, w, h = roi0


# undistorted0 = undistorted0[y: y + h, x: x + w]

x, y, w, h = roi2
# undistorted2 = undistorted2[y: y + h, x: x + w]




cv.imwrite('undistorted0.png', undistorted0)
cv.imwrite('undistorted2.png', undistorted2)

cv.imshow('undistorted0', undistorted0)
cv.imshow('undistorted2', undistorted2)

cv.waitKey(0)

cv.destroyAllWindows()






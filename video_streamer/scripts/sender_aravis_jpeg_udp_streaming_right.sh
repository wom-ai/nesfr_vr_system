#!/bin/sh
set -x
HOST=192.168.0.4
FRAMERATE=36 #72: lagging on Xavier

GST_DEBUG=3 gst-launch-1.0 -v -e aravissrc camera-name=FLIR-1E100147FCEA-0147FCEA ! "video/x-bayer, format=(string)rggb, width=(int)1440, height=(int)1080, framerate=(fraction)$FRAMERATE/1" ! bayer2rgb \
    ! videoconvert ! "video/x-raw, format=(string)RGBA" \
    ! nvvideoconvert ! "video/x-raw(memory:NVMM)" \
    ! nvjpegenc ! rtpjpegpay \
    ! udpsink host=$HOST port=5001 sync=0 \

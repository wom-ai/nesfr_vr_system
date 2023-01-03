#!/bin/sh
set -x
HOST=192.168.0.4
FRAMERATE=36 # 72 lagging on Xavier

GST_DEBUG=3 gst-launch-1.0 -v -e aravissrc camera-name=FLIR-1E100124E18A-0124E18A ! "video/x-bayer, format=(string)rggb, width=(int)1440, height=(int)1080, framerate=(fraction)$FRAMERATE/1" ! bayer2rgb \
    ! nvvideoconvert ! "video/x-raw(memory:NVMM)" \
    ! nvjpegenc quality=85 Enableperf=true ! rtpjpegpay \
    ! udpsink host=$HOST port=5000 sync=0 \

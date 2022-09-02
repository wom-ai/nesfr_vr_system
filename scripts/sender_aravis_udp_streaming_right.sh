#!/bin/sh
set -x
gst-launch-1.0 -v -e aravissrc camera-name=FLIR-1E100147FCEA-0147FCEA ! "video/x-bayer, format=(string)rggb, width=(int)1440, height=(int)1080, framerate=(fraction)72/1" ! bayer2rgb \
    ! videoconvert ! "video/x-raw, format=(string)RGBA" \
    ! nvvideoconvert ! "video/x-raw(memory:NVMM)" \
    ! nvv4l2h264enc maxperf-enable=1 insert-sps-pps=1 idrinterval=30 bitrate=6000000 ! h264parse ! rtph264pay mtu=65000 \
    ! udpsink host=192.168.55.100 port=5001 sync=0 \

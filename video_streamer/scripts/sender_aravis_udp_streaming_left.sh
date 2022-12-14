#!/bin/sh
set -x
HOST=192.168.0.4
FRAMERATE=72

GST_DEBUG=1 gst-launch-1.0 -v -e aravissrc camera-name=FLIR-1E100124E18A-0124E18A ! "video/x-bayer, format=(string)rggb, width=(int)1440, height=(int)1080, framerate=(fraction)$FRAMERATE/1" ! bayer2rgb \
    ! videoconvert ! "video/x-raw, format=(string)RGBA" \
    ! nvvideoconvert ! "video/x-raw(memory:NVMM)" \
    ! nvv4l2h264enc maxperf-enable=1 insert-sps-pps=1 idrinterval=30 bitrate=6000000 MeasureEncoderLatency=true ! h264parse ! rtph264pay mtu=65000 \
    ! udpsink host=$HOST port=5000 sync=0 \

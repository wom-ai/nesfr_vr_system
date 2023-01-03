#!/bin/sh
set -x

gst-launch-1.0 -v -e udpsrc port=5000 ! 'application/x-rtp,encoding-name=H264,payload=96' ! rtph264depay ! avdec_h264 ! textoverlay text=left valignment=top ! xvimagesink sync=0

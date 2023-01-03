#!/bin/sh
set -x

gst-launch-1.0 -v -e udpsrc port=5001 ! "application/x-rtp,media=video,encoding-name=JPEG" ! rtpjpegdepay ! jpegdec ! textoverlay text=right valignment=top ! xvimagesink sync=0

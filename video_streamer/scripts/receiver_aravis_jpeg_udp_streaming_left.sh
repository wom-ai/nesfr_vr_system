#!/bin/sh
set -x

gst-launch-1.0 -v -e udpsrc port=5000 ! "application/x-rtp,media=video,encoding-name=JPEG" ! rtpjpegdepay ! jpegdec ! textoverlay text=left valignment=top ! xvimagesink sync=0

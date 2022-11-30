#!/bin/bash

#set -x

#
# reference
#  - https://superuser.com/questions/1539634/pulseaudio-daemon-wont-start-inside-docker
pulseaudio -D --verbose --exit-idle-time=-1 --system --disallow-exit

#set -e
source /opt/ros/foxy/setup.bash

exec "$@"

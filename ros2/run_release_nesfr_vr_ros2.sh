#!/bin/bash

echo "######################################################################"
echo " run_release_nesfr_vr_ros2.sh"
echo "######################################################################"

pulseaudio -D --verbose --exit-idle-time=-1 --system --disallow-exit

ros2 run nesfr_vr_ros2 nesfr_vr_ros2

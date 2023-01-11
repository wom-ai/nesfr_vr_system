#!/bin/bash

TIMEOUT_IN_SECS=200

echo "[User-Service NESFR VR]"
# https://superuser.com/questions/759759/writing-a-service-that-depends-on-xorg
while [ 1 ]
do
  # Check whether or not socket exists
  if [ -S /tmp/.X11-unix/X0 ]
  then
    echo "[NESFR VR] Xorg is running."
    break
  fi

  # Adjust timeout according to your needs
  if [ $SECONDS -gt $TIMEOUT_IN_SECS ]
  then
    echo "[NESFR VR] Give up waiting for Xorg after $TIMEOUT_IN_SECS secs."
    break
  fi

  echo "[NESFR VR] Wait for Xorg..."
  sleep 5
done

RELEASE_DOCKER_IMAGE=stereoboy/aespa7
docker pull $RELEASE_DOCKER_IMAGE
#
# references
#  - https://stackoverflow.com/questions/24225647/docker-a-way-to-give-access-to-a-host-usb-or-serial-device
#  - https://unix.stackexchange.com/questions/151812/get-device-node-by-major-minor-numbers-pair
#
# $udevadm info --export-db
#
#    --device-cgroup-rule='c 81:* rmw' \ # v4l2
#    --device-cgroup-rule='c 116:* rmw' \ # USB Sound Card: Creative Sound_BlasterX_G1
#
docker run --rm --net=host --runtime nvidia \
    -e DISPLAY=$DISPLAY \
    -v /tmp/.X11-unix/:/tmp/.X11-unix  \
    --device /dev/bus/usb \
    --device /dev/snd \
    --device-cgroup-rule='c 81:* rmw' \
    --device-cgroup-rule='c 116:* rmw' \
    -v /run/udev:/run/udev:ro \
    -v /dev:/dev \
    --name ros2_rover_release \
    $RELEASE_DOCKER_IMAGE:latest /bin/bash -c "/nesfr_vr_ros2_run_release.sh"

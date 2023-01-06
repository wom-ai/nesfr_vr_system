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
docker run -it --rm --net=host --runtime nvidia \
    -e DISPLAY=$DISPLAY \
    -v /tmp/.X11-unix/:/tmp/.X11-unix  \
    --device /dev/bus/usb \
    --device /dev/snd \
    --device-cgroup-rule='c 81:* rmw' \
    --device-cgroup-rule='c 116:* rmw' \
    -v /run/udev:/run/udev:ro \
    -v /dev:/dev \
    -v ${PWD}/..:/work \
    -w /work/nesfr_vr_system/ros2 \
    --name ros2_rover \
    ros2_rover

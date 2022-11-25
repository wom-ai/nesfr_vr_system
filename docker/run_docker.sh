# old ver. setup
#        --device /dev/video0 \ # for webcam
#        --device /dev/bus/usb \ # for flir camera

#
# references
#  - https://stackoverflow.com/questions/24225647/docker-a-way-to-give-access-to-a-host-usb-or-serial-device
#
#        --device /dev/bus/usb \ for flir camera
#        --device-cgroup-rule='c 188:* rmw' \ for flir camera
#        --device-cgroup-rule='c 189:* rmw' \ for usb2serial
#        --device-cgroup-rule='c 81:* rmw' \ for webcam
#

XSOCK=/tmp/.X11-unix
XAUTH=/tmp/.docker.xauth
touch $XAUTH
xauth nlist $DISPLAY | sed -e 's/^..../ffff/' | xauth -f $XAUTH nmerge -


docker run  --gpus '"device=0"' --rm -it \
        --volume=$XSOCK:$XSOCK:rw \
        --volume=$XAUTH:$XAUTH:rw \
        --env="XAUTHORITY=${XAUTH}" \
        --env="DISPLAY=${DISPLAY}" \
        --env QT_X11_NO_MITSHM=1 \
        --env="NVIDIA_DRIVER_CAPABILITIES=all" \
        --device /dev/bus/usb \
        --device-cgroup-rule='c 188:* rmw' \
        --device-cgroup-rule='c 189:* rmw' \
        --device-cgroup-rule='c 81:* rmw' \
        -v /run/udev:/run/udev:ro \
        -v /dev:/dev \
        -v ${PWD}/..:/work \
        -w /work/nesfr_vr_system \
        --name ros2_rover \
   ros2_rover:latest

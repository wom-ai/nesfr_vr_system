#!/bin/bash

echo "######################################################################"
echo " run_release_nesfr_vr_ros2.sh"
echo "######################################################################"

#
# references
#  - https://linuxconfig.org/how-to-propagate-a-signal-to-child-processes-from-a-bash-script
#  - https://stackoverflow.com/questions/7427262/how-to-read-a-file-into-a-variable-in-shell
#
cleanup() {
  echo "######################################################################"
  echo "cleaning up..."
  NESFR_VR_PID=$(<$HOME/.nesfrvr/pids/nesfr_vr.pid)
  echo "kill -s TERM NESFR_VR_PID=$NESFR_VR_PID"
  # Our cleanup code goes here
  kill -s TERM "${NESFR_VR_PID}"
  wait # Wait children's process cleanup
  echo "######################################################################"
}

trap 'echo signal received!; cleanup' INT TERM HUP

echo ">>> set config"
if [ -e "$HOME/.nesfrvr" ] && [ -d "$HOME/.nesfrvr" ]; then
  echo "Found $HOME/.nesfrvr"
  rm -rf $HOME/.nesfrvr/*
else
  echo "Make $HOME/.nesfrvr"
  mkdir -p  $HOME/.nesfrvr
fi

mkdir -p  $HOME/.nesfrvr/pids

mkdir -p  $HOME/.nesfrvr/configs
ln -sf /opt/ros/foxy/share/nesfr_vr_ros2/configs/hw_config_${HOSTNAME}_1280x720.json $HOME/.nesfrvr/configs/hw_config.json
ln -sf /opt/ros/foxy/share/nesfr_vr_ros2/data $HOME/.nesfrvr/data
echo "<<< set config"

pulseaudio -D --verbose --exit-idle-time=-1 --system --disallow-exit

ros2 launch nesfr_vr_ros2 nesfr_vr_with_nesfr7.launch.py &
#ros2 run nesfr_vr_ros2 nesfr_vr_ros2 &

#CHILD_PID="$!"
#wait "${CHILD_PID}"
wait

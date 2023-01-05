#!/bin/bash

echo "######################################################################"
echo " run_release_nesfr_vr_ros2.sh"
echo "######################################################################"

echo ">>> set config"
if [ -e "$HOME/.nesfrvr" ] && [ -d "$HOME/.nesfrvr" ]; then
  echo "Found $HOME/.nesfrvr"
  rm -rf $HOME/.nesfrvr/*
else
  echo "Make $HOME/.nesfrvr"
  mkdir -p  $HOME/.nesfrvr
fi

mkdir -p  $HOME/.nesfrvr/configs
ln -s /opt/ros/foxy/share/nesfr_vr_ros2/configs/hw_config_${HOSTNAME}_1280x720.json $HOME/.nesfrvr/configs/hw_config.json
ln -s /opt/ros/foxy/share/nesfr_vr_ros2/data $HOME/.nesfrvr/data
echo "<<< set config"

pulseaudio -D --verbose --exit-idle-time=-1 --system --disallow-exit

ros2 run nesfr_vr_ros2 nesfr_vr_ros2

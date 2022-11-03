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

/home/wom/jylee/work/nesfr_vr_system/cli/build/nesfr_vr_system_cli

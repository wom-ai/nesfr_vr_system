FROM nvcr.io/nvidia/l4t-base:r35.1.0

ENV DEBIAN_FRONTEND noninteractive

# set ROS_DOMAIN_ID
# https://roboticsbackend.com/ros2-multiple-machines-including-raspberry-pi/
ENV ROS_DOMAIN_ID=5

# install dev tools
COPY ./ros2/nesfr_vr_ros2_entrypoint.sh /
COPY ./ros2/nesfr_vr_ros2_run_release.sh /

COPY ./docker/.tmux.conf /root
RUN apt-get update
RUN apt-get install -y apt-utils
RUN apt-get install -y vim tree htop tmux gdb

# install python3 utils
RUN apt-get install -y python3-pip python3-wheel
RUN pip install --upgrade pip
RUN pip install --upgrade setuptools wheel

# for NESFR VR
# for nmap, arp for network scan
RUN apt-get install -y nmap net-tools
RUN apt-get install -y v4l-utils
RUN apt-get install -y libjsoncpp-dev libspdlog-dev

# configure for nesfr_vr_ros2
#ARG dev_name
#RUN echo "dev_name=${dev_name}"
#RUN mkdir -p /root/.nesfrvr/configs
#COPY ./data/ /root/.nesfrvr/data
#COPY ./configs/hw_config_${dev_name}_1280x720.json  /root/.nesfrvr/configs/hw_config.json

#
# reference for audio sound card
#  - https://askubuntu.com/questions/70560/why-am-i-getting-this-connection-to-pulseaudio-failed-error
RUN apt-get install -y pulseaudio

#
# rerefece for pulseaudio
#  - https://stackoverflow.com/questions/66775654/how-can-i-make-pulseaudio-run-as-root
RUN adduser root pulse-access

# GStreamer
RUN apt-get install -y libgstreamer1.0-dev gstreamer1.0-plugins-base-apps gstreamer1.0-pulseaudio

# ROS2
RUN apt-get install -y software-properties-common
RUN add-apt-repository universe

RUN apt-get update && apt-get install -y curl
RUN curl -sSL https://raw.githubusercontent.com/ros/rosdistro/master/ros.key -o /usr/share/keyrings/ros-archive-keyring.gpg

RUN echo "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/ros-archive-keyring.gpg] http://packages.ros.org/ros2/ubuntu $(. /etc/os-release && echo $UBUNTU_CODENAME) main" | tee /etc/apt/sources.list.d/ros2.list > /dev/null

RUN apt-get update

RUN apt-get install -y ros-foxy-ros-base
RUN apt-get install -y ros-dev-tools
RUN apt-get install -y python3-argcomplete
#RUN apt-get install -y ros-foxy-desktop

ENTRYPOINT ["/nesfr_vr_ros2_entrypoint.sh"]
CMD ["bash"]

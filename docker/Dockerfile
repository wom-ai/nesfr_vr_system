FROM osrf/ros:humble-desktop

ENV DEBIAN_FRONTEND noninteractive

# install dev tools
COPY ./docker/.tmux.conf /root
RUN apt-get update
RUN apt-get install -y apt-utils
RUN apt-get install -y vim tree htop tmux gdb net-tools iputils-ping


# install python3 utils
RUN apt-get install -y python3-pip python3-wheel
RUN pip install --upgrade pip
RUN pip install --upgrade setuptools wheel

# install ROS2 humble
# https://docs.ros.org/en/humble/Installation/Ubuntu-Install-Debians.html
RUN apt-get install -y software-properties-common
RUN add-apt-repository universe

RUN apt-get install -y ros-humble-ros-base
RUN apt-get install -y ros-dev-tools


# install gazebo
# https://gazebosim.org/docs/garden/install_ubuntu
#RUN apt-get install -y lsb-release wget gnupg
#RUN wget https://packages.osrfoundation.org/gazebo.gpg -O /usr/share/keyrings/pkgs-osrf-archive-keyring.gpg
#RUN echo "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/pkgs-osrf-archive-keyring.gpg] http://packages.osrfoundation.org/gazebo/ubuntu-stable $(lsb_release -cs) main" | sudo tee /etc/apt/sources.list.d/gazebo-stable.list > /dev/null

#RUN apt-get update
#RUN apt-get install -y gz-garden

# install gazebo
# https://answers.gazebosim.org/question/28361/how-to-install-gazebo-for-ubuntu-2204-with-ros2-humble/
RUN apt install -y ros-humble-desktop-full
RUN apt install -y ros-humble-gazebo-ros-pkgs

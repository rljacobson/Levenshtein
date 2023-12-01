# Use Ubuntu 18.04 (Bionic) as the base image
FROM ubuntu:bionic

# Essential packages for remote debugging, login, and other development tools
RUN apt-get update && apt-get upgrade -y && apt-get install -y \
    apt-utils \
    gcc \
    g++ \
    openssh-server \
    cmake \
    build-essential \
    gdb \
    gdbserver \
    rsync \
    vim \
    libmysqlclient-dev \
    libboost-all-dev # Add Boost libraries

# Add your code to the container
ADD . /code

# Set your working directory inside the container
WORKDIR /code

# Optional: Expose ports for SSH, gdbserver, or other services
# EXPOSE 22 12345

# Optional: Set a default command or entry point, like launching a server or running a script
# CMD ["./your_script.sh"]

# Use Ubuntu 18.04 (Bionic) as the base image
FROM ubuntu:bionic

# Update the package repository and install essential packages
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
    libboost-all-dev \
    && rm -rf /var/lib/apt/lists/* # Clean up the package list to reduce image size


# Add your code to the container
ADD . /code

# Set your working directory inside the container to /code
WORKDIR /code

# Default command or entry point for the container
CMD ["bash"]

# Use Ubuntu 22.04 (Jammy) as the base image
FROM ubuntu:jammy

# Update the package repository and install essential packages
RUN apt-get update && apt-get upgrade -y && apt-get install -y \
    apt-utils \
    gcc \
    g++ \
    git \
    openssh-server \
    cmake \
    build-essential \
    gdb \
    gdbserver \
    rsync \
    vim \
    libmysqlclient-dev \
    libboost-all-dev \
    wget \
    && rm -rf /var/lib/apt/lists/*

# Verify Git installation
RUN git --version

ap
# Default command or entry point for the container
CMD ["bash"]

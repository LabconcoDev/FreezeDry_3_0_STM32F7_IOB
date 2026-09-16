FROM ghcr.io/zephyrproject-rtos/zephyr-build:v0.29.2

WORKDIR /workspaces

USER root
RUN apt-get update && apt-get install -y \
    usbutils \
    git \
    && rm -rf /var/lib/apt/lists/*

USER user
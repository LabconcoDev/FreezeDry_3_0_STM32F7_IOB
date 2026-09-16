#!/bin/bash
set -e # Exit immediately if any command fails

echo "=== Starting External Zephyr Workspace Initialization ==="

# 1. Create and Initialize the isolated Zephyr RTOS dependencies folder
mkdir -p zephyr_workspace

if [ ! -d "zephyr_workspace/.west" ]; then
    echo "-> Initializing West tracking in 'zephyr_workspace' for Zephyr v4.4.0..."
    cd zephyr_workspace
    west init -m https://github.com/zephyrproject-rtos/zephyr --mr v4.4.0 .
    
    # Configure west to ignore all modules by default, except essential STM32 ones
    echo "-> Configuring module filters for STM32 only..."
    west config update.group-filter -- "+optional,+hal,-*"
    west config update.path-filter -- "+modules/hal/cmsis,+modules/hal/stm32"

    cd ..
else
    echo "-> Zephyr workspace directory already initialized."
fi

echo "-> Updating Zephyr modules (this might take a few minutes)..."
cd zephyr_workspace
west update
west zephyr-export
cd ..

# 2. Scaffold your custom application completely OUTSIDE the workspace
if [ ! -d "my-app" ]; then
    echo "-> Scaffolding external 'my-app' structure..."
    mkdir -p my-app/src

    # Create the top-level CMake build links
    echo "-> Generating my-app/CMakeLists.txt..."
    cat << 'EOF' > my-app/CMakeLists.txt
cmake_minimum_required(VERSION 3.20.0)

# Zephyr requires knowing where the workspace is. 
# We explicitly look for it one level up, inside zephyr_workspace/zephyr
find_package(Zephyr REQUIRED HINTS ../zephyr_workspace/zephyr)
project(stm32f723_app)

target_sources(app PRIVATE src/main.c)
EOF

    # Enable peripheral feature compilation flags
    echo "-> Generating my-app/prj.conf..."
    cat << 'EOF' > my-app/prj.conf
CONFIG_GPIO=y
CONFIG_CONSOLE=y
CONFIG_UART_CONSOLE=y
EOF

    # Write the main entry code
    echo "-> Generating my-app/src/main.c..."
    cat << 'EOF' > my-app/src/main.c
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>

#define LED0_NODE DT_ALIAS(led0)

#if !DT_NODE_HAS_STATUS(LED0_NODE, okay)
#error "Unsupported board: led0 alias is not defined"
#endif

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

int main(void) {
    if (!gpio_is_ready_dt(&led)) {
        printk("Error: Hardware peripheral node not ready.\n");
        return -1;
    }
    
    if (gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE) < 0) {
        return -1;
    }
    
    printk("External STM32F723 App Initialized Successfully!\n");
    
    while (1) {
        gpio_pin_toggle_dt(&led);
        k_msleep(500);
    }
    return 0;
}
EOF

    echo "=== External Application Scaffolding Complete ==="
else
    echo "-> 'my-app' directory already exists. Skipping."
fi
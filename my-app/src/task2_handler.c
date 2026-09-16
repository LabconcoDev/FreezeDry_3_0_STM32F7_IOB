/*
 * Copyright (c) 2026 Labconco Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include "platform/platform_gpio.h"
#include "stack_debug.h"
LOG_MODULE_REGISTER(task2, LOG_LEVEL_INF);

int task2_init(void)
{
    print_stack_usage();
    if (!platform_gpio_is_ready(PLATFORM_GPIO_COLLECTOR_PB1)) {
        return -1;
    }
    return platform_gpio_config_output_inactive(PLATFORM_GPIO_COLLECTOR_PB1);
}

void task2_collector_controller(void)
{
    platform_gpio_toggle(PLATFORM_GPIO_COLLECTOR_PB1);
    //LOG_INF("[Task2] Entered");;
}
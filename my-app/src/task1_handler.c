/*
 * Copyright (c) 2026 Labconco Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include "platform/platform_gpio.h"
#include "stack_debug.h"
LOG_MODULE_REGISTER(task1, LOG_LEVEL_INF);

int task1_init(void)
{
    print_stack_usage();
    if (!platform_gpio_is_ready(PLATFORM_GPIO_VACUUM_PD13)) {
        return -1;
    }
    return platform_gpio_config_output_inactive(PLATFORM_GPIO_VACUUM_PD13);
}

void task1_vacuum_controller(void)
{
    platform_gpio_toggle(PLATFORM_GPIO_VACUUM_PD13);
    //LOG_INF("[Task1] Entered");
}
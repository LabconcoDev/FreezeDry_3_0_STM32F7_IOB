/*
 * Copyright (c) 2026 Labconco Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PLATFORM_GPIO_H_
#define PLATFORM_GPIO_H_

#include <stdbool.h>

typedef enum {
    PLATFORM_GPIO_VACUUM_PD13 = 0,
    PLATFORM_GPIO_COLLECTOR_PB1,
} platform_gpio_id_t;

bool platform_gpio_is_ready(platform_gpio_id_t id);
int platform_gpio_config_output_inactive(platform_gpio_id_t id);
int platform_gpio_toggle(platform_gpio_id_t id);

#endif /* PLATFORM_GPIO_H_ */
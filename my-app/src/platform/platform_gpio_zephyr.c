/*
 * Copyright (c) 2026 Labconco Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include "platform_gpio.h"
#include <errno.h>
#include <zephyr/drivers/gpio.h>

/* Use standard aliases mapped directly from your .dts file */
static const struct gpio_dt_spec vacuum_pc4 = GPIO_DT_SPEC_GET(DT_ALIAS(vacuum_gpio), gpios);
static const struct gpio_dt_spec collector_pb1 = GPIO_DT_SPEC_GET(DT_ALIAS(collector_gpio), gpios);

static const struct gpio_dt_spec *get_spec(platform_gpio_id_t id)
{
    if (id == PLATFORM_GPIO_VACUUM_PD13) return &vacuum_pc4;
    if (id == PLATFORM_GPIO_COLLECTOR_PB1) return &collector_pb1;
    return NULL;
}

bool platform_gpio_is_ready(platform_gpio_id_t id)
{
    const struct gpio_dt_spec *spec = get_spec(id);
    return (spec != NULL) && gpio_is_ready_dt(spec);
}

int platform_gpio_config_output_inactive(platform_gpio_id_t id)
{
    const struct gpio_dt_spec *spec = get_spec(id);
    if (!spec) return -EINVAL;
    return gpio_pin_configure_dt(spec, GPIO_OUTPUT_INACTIVE);
}

int platform_gpio_toggle(platform_gpio_id_t id)
{
    const struct gpio_dt_spec *spec = get_spec(id);
    if (!spec) return -EINVAL;
    return gpio_pin_toggle_dt(spec);
}
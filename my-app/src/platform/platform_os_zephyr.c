/*
 * Copyright (c) 2026 Labconco Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include "platform_os.h"
#include <zephyr/kernel.h>

void platform_thread_sleep_ms(uint32_t ms)
{
    k_msleep(ms);
}

uint32_t platform_get_time_ms(void)
{
    return (uint32_t)k_uptime_get();
}
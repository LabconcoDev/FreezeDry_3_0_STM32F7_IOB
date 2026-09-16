/*
 * Copyright (c) 2026 Labconco Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PLATFORM_OS_H_
#define PLATFORM_OS_H_

#include <stdint.h>

/* Timing Helpers */
void platform_thread_sleep_ms(uint32_t ms);
uint32_t platform_get_time_ms(void);

#endif /* PLATFORM_OS_H_ */
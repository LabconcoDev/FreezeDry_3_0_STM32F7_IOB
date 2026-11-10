/*
 * Copyright (c) 2026 Labconco Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TASK_MANAGER_H_
#define TASK_MANAGER_H_

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================
 * TASK START API
 * ========================================================= */
void start_comm_tasks(void);
void start_process_tasks(void);
void start_all_tasks(void);
void reset_timeline_trace(void);
int task2_init(void);
int dma_uart_display_init(void);
int dma_uart_accessory_iob_init(void);


void task2_collector_controller(void);
void dma_uart_display_run(void);
void dma_uart_accessory_iob_run(void);

#ifdef __cplusplus
}
#endif

#endif /* TASK_MANAGER_H_ */
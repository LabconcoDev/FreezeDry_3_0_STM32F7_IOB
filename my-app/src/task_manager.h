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
 * TASK START API (ONLY WHAT SYSTEM NEEDS)
 * ========================================================= */
void start_all_tasks(void);
/* Task start functions */
int vacuum_start(void);
int collector_start(void);
int smf_start(void);
int test5_start(void);

/* Optional: telemetry task */
int telemetry_start(void);

/* =========================================================
 * LOW LEVEL INIT FUNCTIONS (USED INSIDE START ONLY)
 * ========================================================= */
int task1_init(void);
int task2_init(void);
int task3_init(void);
int task5_init(void);
int uart_telemetry_init(void);
int dma_uart_display_init(void);
int dma_uart_accessory_iob_init(void);

/* =========================================================
 * TASK LOOP FUNCTIONS (RUN INSIDE THREADS)
 * ========================================================= */
void task1_vacuum_controller(void);
void task2_collector_controller(void);
void task3_smf(void);
void task5_vacuum_controller(void);
void uart_telemetry_handler(void);
void dma_uart_display_run(void);
void dma_uart_accessory_iob_run(void);


#ifdef __cplusplus
}
#endif

#endif /* TASK_MANAGER_H_ */
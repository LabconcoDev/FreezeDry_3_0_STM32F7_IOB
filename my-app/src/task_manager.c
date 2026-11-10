#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "task_manager.h"
#include "task_ads124s08.h"
#include "task_telemetry.h"
#include "task_dma_uart_display.h"
#include "task_control.h" 
#include "task_sensor.h"
#include <string.h>
LOG_MODULE_REGISTER(task_manager, LOG_LEVEL_INF);


#include <zephyr/sys/atomic.h>

/* =========================================================
 * SCHEDULER TIMELINE RECORDER (Captures first 30 thread runs)
 * ========================================================= */
#define MAX_TRACE_EVENTS 30

struct trace_event {
    const char *thread_name;
    uint32_t start_ms;
    uint32_t duration_us;
};

static struct trace_event timeline_log[MAX_TRACE_EVENTS];
static atomic_t trace_idx = ATOMIC_INIT(0);
static uint32_t scheduler_zero_ms = 0; // Reference t = 0 ms when all threads start



// Forward declaration of our shared worker wrapper
static void task_worker_wrapper(void *p1, void *p2, void *p3);

/* =========================================================
 * 1. TRUE COMPILE-TIME THREAD CREATION
 * ========================================================= */
#define TASK_STACK_SIZE_1024 1024
#define TASK_STACK_SIZE_2048 2048
/*
 * K_THREAD_DEFINE creates a Zephyr thread at compile time.
 * Parameters: thread ID, stack size, entry function, 3 arguments,
 * priority(-ve : Cooperative, +ve : Preemptive scheduling), options, and initial delay.
 *
 * SYS_FOREVER_MS means the thread is created but does not start
 * automatically. It is explicitly started later using k_thread_start().
 */

/* 1. Process Control Thread (Cooperative, Priority -1, 100 ms) */
K_THREAD_DEFINE(control_tid,        TASK_STACK_SIZE_2048, task_worker_wrapper, NULL, NULL, NULL, -1, 0, SYS_FOREVER_MS);
/* 2. Display Comms Threads (Preemptive, Priority 1, Event-Driven) */
K_THREAD_DEFINE(comm_display_tid,   TASK_STACK_SIZE_2048, task_worker_wrapper, NULL, NULL, NULL,  1, 0, SYS_FOREVER_MS);
K_THREAD_DEFINE(dma_tx_tid,         TASK_STACK_SIZE_2048, task_worker_wrapper, NULL, NULL, NULL,  1, 0, SYS_FOREVER_MS);
/* 3. Accessory Comms Thread (Preemptive, Priority 2, Event-Driven) */
K_THREAD_DEFINE(comm_accessory_tid, TASK_STACK_SIZE_2048, task_worker_wrapper, NULL, NULL, NULL,  2, 0, SYS_FOREVER_MS);
/* 4. All Sensors & Bus Thread (Preemptive, Priority 3, 50 ms) */
K_THREAD_DEFINE(sensor_tid,         TASK_STACK_SIZE_2048, task_worker_wrapper, NULL, NULL, NULL,  3, 0, SYS_FOREVER_MS);
/* 5. Safety & Alerts Thread (Preemptive, Priority 4, 100 ms) */
K_THREAD_DEFINE(alert_monitor_tid,  TASK_STACK_SIZE_2048, task_worker_wrapper, NULL, NULL, NULL,  4, 0, SYS_FOREVER_MS);
/* 6. System & Telemetry Thread (Preemptive, Priority 5, 20 ms / 1000 ms sim) */
K_THREAD_DEFINE(system_ui_tid,      TASK_STACK_SIZE_2048, task_worker_wrapper, NULL, NULL, NULL,  5, 0, SYS_FOREVER_MS);


/* =========================================================
 * 2. CONFIGURATION STRUCTURE & TABLE
 * ========================================================= */
typedef int (*task_init_fn_t)(void);
typedef void (*task_handler_fn_t)(void);

struct app_task {
    const char *name;
    task_init_fn_t init;
    task_handler_fn_t handler;
    int interval_ms;
    k_tid_t thread_id; // Links this configuration row to the static thread ID above
    uint32_t max_exec_us;
    uint32_t run_count;
};

static struct app_task task_table[] = {
    {
        .name        = "1_Control_Thread",
        .init        = control_task_init,               // Later: control_thread_init
        .handler     = control_task_run,  // Later: control_thread_run (Collector + Vac + Lexsol)
        .interval_ms = 100,                      // 100 ms loop
        .thread_id   = control_tid,
    },
    {
        .name        = "2a_Comm_Display_RX",
        .init        = dma_uart_display_init,
        .handler     = dma_uart_display_run,     // Blocks on rx_msgq (K_FOREVER)
        .interval_ms = 0,                        // Event-driven
        .thread_id   = comm_display_tid,
    },
    {
        .name        = "2b_Comm_Display_TX",
        .init        = NULL,
        .handler     = dma_uart_tx_thread,       // Blocks on tx_msgq (K_FOREVER)
        .interval_ms = 0,                        // Event-driven
        .thread_id   = dma_tx_tid,
    },
    {
        .name        = "3_Comm_Accessory",
        .init        = dma_uart_accessory_iob_init,
        .handler     = dma_uart_accessory_iob_run,
        .interval_ms = 0,                        // Event-driven
        .thread_id   = comm_accessory_tid,
    },
    {
        .name        = "4_Sensor_Thread",
        .init        = sensor_task_init,            // Initializes RTD + Sim sensors
        .handler     = sensor_task_run,             // Generates simulated Triad sensor data!
        .interval_ms = 50,                       // 50 ms loop
        .thread_id   = sensor_tid,
    },
    {
        .name        = "5_Alert_Monitor",
        .init        = task2_init,               // Placeholder init
        .handler     = task2_collector_controller,// Later: alert_monitor_run
        .interval_ms = 100,                      // 100 ms loop
        .thread_id   = alert_monitor_tid,
    },
    {
        .name        = "6_System_UI_Telemetry",
        .init        = telemetry_task_init,
        .handler     = telemetry_task_run,       // Streams/pushes Triad telemetry to Display!
        .interval_ms = 20,                       // 20 ms loop
        .thread_id   = system_ui_tid,
    },
};

#define NUM_TASKS ARRAY_SIZE(task_table)

/* =========================================================
 * 3. WORKER WRAPPER (The shared execution loop)
 * ========================================================= */
static void task_worker_wrapper(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3);
    
    struct app_task *task = NULL;
    k_tid_t my_id = k_current_get();

    // Dynamically match this running thread to its configuration table entry
    for (size_t i = 0; i < NUM_TASKS; i++) 
    {
        if (task_table[i].thread_id == my_id) 
        {
            task = &task_table[i];
            break;
        }
    }

    if (!task) 
    {
        printk("[CRITICAL] Thread %p failed to map to a configuration context!\n", my_id);
        return;
    }

    printk("\n[THREAD ENTERED] %s\n", task->name);
    // Master brain of periodic task running
    while (1) 
    {
        // 1. Capture exact start time (relative to scheduler_zero_ms) and CPU cycle counter
        uint32_t t_start_ms = k_uptime_get_32() - scheduler_zero_ms;
        uint32_t cyc_start  = k_cycle_get_32();

        // 2. Run the specific task logic
        task->handler();

        // 3. Calculate how many microseconds the handler took
        uint32_t dur_us = k_cyc_to_us_floor32(k_cycle_get_32() - cyc_start);

        // 4. Record periodic threads (interval_ms > 0) in RAM without UART delay
        if (task->interval_ms > 0) 
        {
            long idx = atomic_inc(&trace_idx);
            if (idx < MAX_TRACE_EVENTS) 
            {
                timeline_log[idx].thread_name = task->name;
                timeline_log[idx].start_ms    = t_start_ms;
                timeline_log[idx].duration_us = dur_us;
            } 
            else if (idx == MAX_TRACE_EVENTS) 
            {
                // Buffer is full! Print the entire captured timeline once:
                printk("\n================ ZEPHYR SCHEDULER TIMELINE ================\n");
                for (int i = 0; i < MAX_TRACE_EVENTS; i++) 
                {
                    printk("t = %4u ms | %-24s | exec = %5u us\n",
                           timeline_log[i].start_ms,
                           timeline_log[i].thread_name,
                           timeline_log[i].duration_us);
                }
                printk("===========================================================\n\n");
                // atomic_set(&trace_idx, 0);
            }
        }

        // Inside while(1) in task_worker_wrapper():
        task->run_count++;
        if (dur_us > task->max_exec_us) {
            task->max_exec_us = dur_us; // Always catches the worst-case spike, even between prints!
        }

        // 5. Sleep until next period
        k_msleep(task->interval_ms);
    }
}


/* =========================================================
 * 4. Task Start Up (Staged Boot Helpers)
 * ========================================================= */

/* Stage 1 Helper: Start ONLY the Event-Driven UART DMA Comms Threads (interval_ms == 0) */
void start_comm_tasks(void)
{
    for (size_t i = 0; i < NUM_TASKS; i++) 
    {
        struct app_task *task = &task_table[i];

        if (task->interval_ms == 0) 
        {
            if (task->init != NULL) 
            {
                if (task->init() < 0) 
                {
                    LOG_ERR("[%s] init failed! Thread will remain suspended.", task->name);
                    continue;
                }
            }
            k_thread_name_set(task->thread_id, task->name);
            k_thread_start(task->thread_id);
            LOG_INF("Started Comms Thread: %s", task->name);
        }
    }
}

/* Stage 2 Helper: Initialize & Start Periodic Tasks synchronized at t = 0 ms */
void start_process_tasks(void)
{
    /* 1. Run peripheral init() for all periodic tasks first */
    for (size_t i = 0; i < NUM_TASKS; i++) 
    {
        struct app_task *task = &task_table[i];

        if (task->interval_ms > 0) 
        {
            if (task->init != NULL) 
            {
                if (task->init() < 0) 
                {
                    LOG_ERR("[%s] init failed!", task->name);
                    continue;
                }
            }
            k_thread_name_set(task->thread_id, task->name);
            LOG_INF("Initialized Periodic Task: %s", task->name);
        }
    }

    /* 2. Lock scheduler so all periodic threads release at the exact same t = 0 ms tick */
    k_sched_lock();
    scheduler_zero_ms = k_uptime_get_32();

    for (size_t i = 0; i < NUM_TASKS; i++) 
    {
        struct app_task *task = &task_table[i];

        if (task->interval_ms > 0) 
        {
            k_thread_start(task->thread_id);
        }
    }
    k_sched_unlock();
    LOG_INF("All Periodic Tasks released at t = 0 ms");
}

/* Legacy wrapper if anything calls start_all_tasks() */
void start_all_tasks(void)
{
    start_comm_tasks();
    start_process_tasks();
}

void reset_timeline_trace(void)
{
    printk("\n--- THREAD STATS (Total Runs & Worst-Case Exec Time) ---\n");
    for (size_t i = 0; i < NUM_TASKS; i++) {
        printk("%-24s | Runs: %6u | Max Exec: %5u us\n",
               task_table[i].name,
               task_table[i].run_count,
               task_table[i].max_exec_us);
    }
    scheduler_zero_ms = k_uptime_get_32(); // Reset window t = 0 ms for next snapshot
    atomic_set(&trace_idx, 0);             // Trigger next 30-event timeline capture
}
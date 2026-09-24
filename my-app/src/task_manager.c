#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "task_manager.h"
#include "task_ads124s08.h"
#include "task_telemetry.h"
#include <string.h>
LOG_MODULE_REGISTER(task_manager, LOG_LEVEL_INF);

// Forward declaration of our shared worker wrapper
static void task_worker_wrapper(void *p1, void *p2, void *p3);

/* =========================================================
 * 1. TRUE COMPILE-TIME THREAD CREATION
 * Changed K_FOREVER to SYS_FOREVER_MS to fix compiler macro error
 * ========================================================= */
#define TASK_STACK_SIZE_1024 1024

K_THREAD_DEFINE(vacuum_tid,             TASK_STACK_SIZE_1024, task_worker_wrapper, NULL, NULL, NULL, 6, 0, SYS_FOREVER_MS);
K_THREAD_DEFINE(collector_tid,          TASK_STACK_SIZE_1024, task_worker_wrapper, NULL, NULL, NULL, 7, 0, SYS_FOREVER_MS);
K_THREAD_DEFINE(smf_tid,                TASK_STACK_SIZE_1024, task_worker_wrapper, NULL, NULL, NULL, 5, 0, SYS_FOREVER_MS);
K_THREAD_DEFINE(test5_tid,              TASK_STACK_SIZE_1024, task_worker_wrapper, NULL, NULL, NULL, 3, 0, SYS_FOREVER_MS);
K_THREAD_DEFINE(dma_tx_tid,             TASK_STACK_SIZE_1024, task_worker_wrapper, NULL, NULL, NULL, 2, 0, SYS_FOREVER_MS);
K_THREAD_DEFINE(ads_tid,                TASK_STACK_SIZE_1024, task_worker_wrapper, NULL, NULL, NULL, 2, 0, SYS_FOREVER_MS);
K_THREAD_DEFINE(acc_uart_tid,           TASK_STACK_SIZE_1024, task_worker_wrapper, NULL, NULL, NULL, 2, 0, SYS_FOREVER_MS);
K_THREAD_DEFINE(display_telemetry_tid,  TASK_STACK_SIZE_1024, task_worker_wrapper, NULL, NULL, NULL, 2, 0, SYS_FOREVER_MS);


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
};

static struct app_task task_table[] = {
    {
        .name        = "Task1: Vacuum",
        .init        = task1_init,
        .handler     = task1_vacuum_controller,
        .interval_ms = 5000,
        .thread_id   = vacuum_tid,
    },
    {
        .name        = "Task2: Collector",
        .init        = task2_init,
        .handler     = task2_collector_controller,
        .interval_ms = 6000,
        .thread_id   = collector_tid,
    },
    {
        .name        = "Task3: SMF",
        .init        = task3_init,
        .handler     = task3_smf,
        .interval_ms = 7000,
        .thread_id   = smf_tid,
    },

    {
        .name        = "Task4: DMA UART Display",
        .init        = dma_uart_display_init,   
        .handler     = dma_uart_display_run,    
        .interval_ms = 1000,                    // Runs every 1000ms
        .thread_id   = dma_tx_tid,
    },

    {
        .name        = "Task5: Test",
        .init        = task5_init,
        .handler     = task5_vacuum_controller,
        .interval_ms = 8000,
        .thread_id   = test5_tid,
    },

    {
        .name        = "Task6: ADS124S08 ADC",
        .init        = ads_task_init,
        .handler     = ads_task_run,
        .interval_ms = 1000,
        .thread_id   = ads_tid,
    },
        
    {
        .name        = "Task7: DMA UART Accessory",
        .init        = dma_uart_accessory_iob_init,   
        .handler     = dma_uart_accessory_iob_run,    
        .interval_ms = 1000,
        .thread_id   = acc_uart_tid,
    },
        
    {
        .name        = "Task8: UART Telemetry Stream",
        .init        = telemetry_task_init,
        .handler     = telemetry_task_run,
        .interval_ms = 1000,
        .thread_id   = display_telemetry_tid,
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
    for (size_t i = 0; i < NUM_TASKS; i++) {
        if (task_table[i].thread_id == my_id) {
            task = &task_table[i];
            break;
        }
    }

    if (!task) {
        printk("[CRITICAL] Thread %p failed to map to a configuration context!\n", my_id);
        return;
    }

    printk("\n[THREAD ENTERED] %s\n", task->name);


    while (1) {
        //printk("[THREAD LOOP] %s count=%d\n", task->name, counter++);

        size_t unused = 0;
        k_thread_stack_space_get(task->thread_id, &unused);
        printk("[STACK UNUSED] %s = %zu\n", task->name, unused);

        // Run the specific task logic
        task->handler();

        printk("[THREAD AFTER HANDLER] %s\n", task->name);
        k_msleep(task->interval_ms);
    }
}

/* =========================================================
 * 4. PUBLIC INTERFACE
 * ========================================================= */
void start_all_tasks(void)
{
    LOG_INF("Initializing and starting all static tasks...");

    for (size_t i = 0; i < NUM_TASKS; i++) {
        struct app_task *task = &task_table[i];
        
        LOG_INF("Starting %s", task->name);

        // 1. Run peripheral hardware init safely
        if (task->init() < 0) {
            LOG_ERR("[%s] init failed! Thread will remain suspended.", task->name);
            continue; // Skip starting this thread to protect the system
        }
        LOG_INF("%s init OK", task->name);

        // 2. Assign thread name and wake it up safely 
        k_thread_name_set(task->thread_id, task->name);
        k_thread_start(task->thread_id);
        
        LOG_INF("%s thread successfully un-suspended", task->name);
    }

    LOG_INF("All operational tasks processed");
}
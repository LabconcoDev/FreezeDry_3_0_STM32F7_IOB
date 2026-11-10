#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/smf.h>
#include "task_control.h"
#include "boot_sequence.h"
#include "product_type.h"

LOG_MODULE_REGISTER(task_control, LOG_LEVEL_INF);

/* =========================================================
 * PROCESS CONTROL SUB-MACHINES (Row 1 of Architecture Table)
 * ========================================================= */
static void collector_smf_run(void)
{
    /* Collector, Defrost & -105C Compressors */
}

static void vacuum_smf_run(void)
{
    /* Vacuum Pump, Purge & Vent Valve PID */
}

static void lexsol_smf_run(void)
{
    /* TRIAD Lexsol Pump, Pre-Freeze Ref Valve & Shelf Heater */
}

static void mode_smf_run(void)
{
    /* Auto-Start, Manual & Program Modes + End-Point Detection */
}

/* =========================================================
 * THREAD INIT & 100 MS RUNNER (1_Control_Thread)
 * ========================================================= */
int control_task_init(void)
{
    LOG_INF("1_Control_Thread SMF template initialized");
    return 0;
}

void control_task_run(void)
{
    /* 1. Common Process Controllers (FD & TRIAD) */
    collector_smf_run();
    vacuum_smf_run();
    mode_smf_run();

    /* 2. TRIAD-Only Process Controller */
    if (u8_ProductID == IAM_TRIAD) 
    {
        lexsol_smf_run();
    }
}
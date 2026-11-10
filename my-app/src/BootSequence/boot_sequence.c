#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "boot_sequence.h"
#include "task_manager.h"
#include "product_type.h"
#include "main_iob_data.h"
#include "accessory_iob_data.h"

LOG_MODULE_REGISTER(boot_sequence, LOG_LEVEL_INF);

/* Global Product & Accessory IDs (used across DataStorage, Telemetry, and Handlers) */
uint8_t u8_ProductID   = IAM_UNKNOWN;
uint8_t u8_AccessoryID = IAM_UNKNOWN;

/* External link to pushIamAlive() defined in src/communication/sensor_handler.c */
extern void pushIamAlive(void);

/* =========================================================
 * STAGE 1: Comms Threads, Hardware Discovery, Buffers & IAM_ALIVE
 * ========================================================= */
void boot_stage1_comms_and_discovery(void)
{
    LOG_INF("--- [BOOT STAGE 1] Starting Comms & Discovery ---");

    /* 1. Start Event-Driven Comms Threads first (UART7 Display & UART4 Accessory) */
    start_comm_tasks();

    /* 2. Determine Main IOB Product Type (IAM_TRIAD vs IAM_FD) */
    u8_ProductID = FindProductType();

    /* 3. Determine Accessory IOB Type over UART4 (Only FD has external BT/ST accessories) */
    if (u8_ProductID == IAM_FD) 
    {
        u8_AccessoryID = FindAccessoryType();
    } 
    else 
    {
        u8_AccessoryID = IAM_NO_ACCESSORY; /* TRIAD is an all-in-one system */
    }

    /* 4. Initialize Product- & Accessory-specific Shadow Memory buffers (0x7FFF defaults) */
    MainIOBMemoryAllocation();
    AccessoryData_Init();
    MemoryAllocation();

    /* 5. Transmit Proactive Boot Announcement (SYSTEM_IAM_ALIVE) to Display on UART7 */
    pushIamAlive();

    LOG_INF("--- [BOOT STAGE 1] Complete (ProductID=0x%02X, AccessoryID=0x%02X) ---",
            u8_ProductID, u8_AccessoryID);
}

/* =========================================================
 * STAGE 2: Start Periodic Real-Time Tasks
 * ========================================================= */
void boot_stage2_periodic_tasks(void)
{
    LOG_INF("--- [BOOT STAGE 2] Starting Periodic Tasks ---");

    /* Initialize and start Control, Sensor, Alert, and Telemetry threads at t = 0 ms */
    start_process_tasks();

    LOG_INF("--- [BOOT STAGE 2] All Periodic Tasks Running ---");
}

/* =========================================================
 * MASTER BOOT SEQUENCE (Called from main.c)
 * ========================================================= */
void system_boot_sequence(void)
{

    LOG_INF("BOOT: ENTER");

    LOG_INF("BOOT: BEFORE STAGE 1");
    boot_stage1_comms_and_discovery();
    LOG_INF("BOOT: AFTER STAGE 1");

    LOG_INF("BOOT: BEFORE STAGE 2");
    boot_stage2_periodic_tasks();
    LOG_INF("BOOT: AFTER STAGE 2");

    LOG_INF("BOOT: COMPLETE");
    LOG_INF("================ SYSTEM BOOT DONE =================");




}
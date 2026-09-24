
/*The structures here would be updated from the sensor directly*/

#include "main_iob_data.h"
#include "product_type.h"
#include "communication/protocol_defs.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(main_iob_data, LOG_LEVEL_INF);

extern uint8_t u8_ProductID;
MainIOBShadowMemory_u MainIOBShadowMemory;

/* 2. Define the Mutex to protect against Race Conditions */
K_MUTEX_DEFINE(shadow_memory_mutex);


void MainIOBMemoryAllocation(void)
{
    // Lock the memory before wiping it
    k_mutex_lock(&shadow_memory_mutex, K_FOREVER);
   
    memset(&MainIOBShadowMemory, 0, sizeof(MainIOBShadowMemory));

    if (u8_ProductID == IAM_FD)
    {

        /*
         * FD:
         * 1 Moisture
         * 1 System Vacuum
         * 6 Sample Vacuum
         * 1 Collector Temperature
         * 1 Shell Dry Chamber Temperature
         */
        /* FD SENSORS */
        MainIOBShadowMemory.FD.Digital_Sensors.Moisture = MOISTURE_NO;
        
        MainIOBShadowMemory.FD.Vacuum_Sensors.SystemVacuum.state.vacuum = 0x7FFF;
        
        for (int i = 0; i < 6; i++) 
        {
            MainIOBShadowMemory.FD.Vacuum_Sensors.SampleVacuum[i].state.vacuum = 0x7FFF;
        }
        MainIOBShadowMemory.FD.ADC_Sensors.CollectorTemp.temperature       = 0x7FFF;
        MainIOBShadowMemory.FD.ADC_Sensors.ShellDryChamberTemp.temperature = 0x7FFF;
    }

    if (u8_ProductID == IAM_TRIAD)
    {
        /*
         * TRIAD:
         * 1 System Vacuum
         * 1 Collector Temperature
         * 1 Shell Dry Chamber Temperature
         * 1 Triad Shelf Temperature
         * 1 Triad Sample Temperature
         * 1 Tilt Switch
         */
        /* TRIAD SENSORS */
        MainIOBShadowMemory.TRIAD.ADC_Sensors.CollectorTemp.temperature = 0x7FFF;
        MainIOBShadowMemory.TRIAD.ADC_Sensors.ShellDryChamberTemp.temperature = 0x7FFF;
        MainIOBShadowMemory.TRIAD.ADC_Sensors.ShelfTemp.temperature = 0x7FFF;
        MainIOBShadowMemory.TRIAD.ADC_Sensors.SampleTemp.temperature = 0x7FFF;
        
        MainIOBShadowMemory.TRIAD.Vacuum_Sensors.SystemVacuum.state.vacuum = 0x7FFF;
        MainIOBShadowMemory.TRIAD.Digital_Sensors.TiltSwitch = SWITCH_OPEN;
    }
    k_mutex_unlock(&shadow_memory_mutex);
}

/* ======================================================== */
/* UNIVERSAL DATA UPDATER                                   */
/* ======================================================== */
void UpdateMainIOBData(uint8_t action, const uint8_t *data)
{
    // 1. Lock the Mutex so no other task can interrupt us while writing
    k_mutex_lock(&shadow_memory_mutex, K_FOREVER);
    if (u8_ProductID == IAM_TRIAD) 
    {
        switch (action) 
        {
            /* --- ADC TASK SENSORS --- */
            case SENSOR_COLLECTOR_TEMP:
                memcpy(MainIOBShadowMemory.TRIAD.ADC_Sensors.CollectorTemp.raw, data, 2);
                MainIOBShadowMemory.TRIAD.ADC_Sensors.Last_Update_ms = k_uptime_get_32();
                break;
            case SENSOR_SHELL_DRY_LEXSOL: // (Shell Chamber Temp)
                memcpy(MainIOBShadowMemory.TRIAD.ADC_Sensors.ShellDryChamberTemp.raw, data, 2);
                MainIOBShadowMemory.TRIAD.ADC_Sensors.Last_Update_ms = k_uptime_get_32();
                break;
            case SENSOR_SHELF1_TEMP:
                memcpy(MainIOBShadowMemory.TRIAD.ADC_Sensors.ShelfTemp.raw, data, 2);
                MainIOBShadowMemory.TRIAD.ADC_Sensors.Last_Update_ms = k_uptime_get_32();
                break;
            case SENSOR_SAMPLE1_TEMP:
                memcpy(MainIOBShadowMemory.TRIAD.ADC_Sensors.SampleTemp.raw, data, 2);
                MainIOBShadowMemory.TRIAD.ADC_Sensors.Last_Update_ms = k_uptime_get_32();
                break;
            /* --- VACUUM TASK SENSORS --- */
            case SENSOR_SYSTEM_VAC:
                // Note: Vacuum is 4 bytes, not 2 bytes!
                memcpy(MainIOBShadowMemory.TRIAD.Vacuum_Sensors.SystemVacuum.raw, data, 4);
                // Note: Updates the VACUUM timestamp, not the ADC timestamp!
                MainIOBShadowMemory.TRIAD.Vacuum_Sensors.Last_Update_ms = k_uptime_get_32();
                break;
            /* --- DIGITAL / GPIO SENSORS --- */
            case SENSOR_TILT_SW:
                MainIOBShadowMemory.TRIAD.Digital_Sensors.TiltSwitch = data[0];
                MainIOBShadowMemory.TRIAD.Digital_Sensors.Last_Update_ms = k_uptime_get_32();
                break;
            default:
                LOG_WRN("UpdateMainIOBData: Unknown TRIAD action 0x%02X", action);
                break;
        }
    }
    else if (u8_ProductID == IAM_FD)
    {
        // update FD sensor info
    }
    // 2. We are done writing. Unlock the Mutex so other tasks can read/write.
    k_mutex_unlock(&shadow_memory_mutex);
}
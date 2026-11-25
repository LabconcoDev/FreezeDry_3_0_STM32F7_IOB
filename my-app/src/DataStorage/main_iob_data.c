/* The structures here are updated from the sensor tasks directly */

#include "main_iob_data.h"
#include "product_type.h"
#include "communication/protocol_defs.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(main_iob_data, LOG_LEVEL_INF);

extern uint8_t u8_ProductID;
MainIOBShadowMemory_u MainIOBShadowMemory;

/* Mutex to protect against race conditions */
K_MUTEX_DEFINE(shadow_memory_mutex);

void MainIOBMemoryAllocation(void)
{
    k_mutex_lock(&shadow_memory_mutex, K_FOREVER);

    memset(&MainIOBShadowMemory, 0, sizeof(MainIOBShadowMemory));

    if (u8_ProductID == IAM_FD)
    {
        MainIOBShadowMemory.FD.Digital_Sensors.Moisture                        = MOISTURE_NO;
        MainIOBShadowMemory.FD.Vacuum_Sensors.SystemVacuum.state.vacuum        = 0x7FFF;

        for (int i = 0; i < 6; i++)
        {
            MainIOBShadowMemory.FD.Vacuum_Sensors.SampleVacuum[i].state.vacuum = 0x7FFF;
        }
        for (int i = 0; i < 2; i++)
        {
            MainIOBShadowMemory.FD.Vacuum_Sensors.EndPointAddonVacuum[i].state.vacuum = 0x7FFF;
        }

        MainIOBShadowMemory.FD.ADC_Sensors.CollectorTemp.temperature           = 0x7FFF;
        MainIOBShadowMemory.FD.ADC_Sensors.AmbientTemp.temperature             = 0x7FFF;
        MainIOBShadowMemory.FD.ADC_Sensors.FirstStageHxOutletTemp.temperature  = 0x7FFF;
        MainIOBShadowMemory.FD.ADC_Sensors.SecondStageHxOutletTemp.temperature = 0x7FFF;
        MainIOBShadowMemory.FD.ADC_Sensors.FirstStageSuctionTemp.temperature   = 0x7FFF;
        MainIOBShadowMemory.FD.ADC_Sensors.SecondStageSuctionTemp.temperature  = 0x7FFF;
        MainIOBShadowMemory.FD.ADC_Sensors.ShellFreezerTemp.temperature        = 0x7FFF;
        MainIOBShadowMemory.FD.ADC_Sensors.MiniChamberTemp.temperature         = 0x7FFF;
    }

    if (u8_ProductID == IAM_TRIAD)
    {
        MainIOBShadowMemory.TRIAD.ADC_Sensors.CollectorTemp.temperature           = 0x7FFF;
        MainIOBShadowMemory.TRIAD.ADC_Sensors.AmbientTemp.temperature             = 0x7FFF;
        MainIOBShadowMemory.TRIAD.ADC_Sensors.LexsolTemp.temperature              = 0x7FFF;
        MainIOBShadowMemory.TRIAD.ADC_Sensors.FirstStageHxOutletTemp.temperature  = 0x7FFF;
        MainIOBShadowMemory.TRIAD.ADC_Sensors.SecondStageHxOutletTemp.temperature = 0x7FFF;
        MainIOBShadowMemory.TRIAD.ADC_Sensors.FirstStageSuctionTemp.temperature   = 0x7FFF;
        MainIOBShadowMemory.TRIAD.ADC_Sensors.SecondStageSuctionTemp.temperature  = 0x7FFF;
        MainIOBShadowMemory.TRIAD.ADC_Sensors.ShelfTemp.temperature               = 0x7FFF;
        MainIOBShadowMemory.TRIAD.ADC_Sensors.SampleTemp.temperature              = 0x7FFF;

        MainIOBShadowMemory.TRIAD.Vacuum_Sensors.SystemVacuum.state.vacuum        = 0x7FFF;
        for (int i = 0; i < 6; i++)
        {
            MainIOBShadowMemory.TRIAD.Vacuum_Sensors.SampleVacuum[i].state.vacuum = 0x7FFF;
        }
        for (int i = 0; i < 2; i++)
        {
            MainIOBShadowMemory.TRIAD.Vacuum_Sensors.EndPointAddonVacuum[i].state.vacuum = 0x7FFF;
        }

        MainIOBShadowMemory.TRIAD.Digital_Sensors.TiltSwitch = SWITCH_OPEN;
    }

    k_mutex_unlock(&shadow_memory_mutex);
}

/* ======================================================== */
/* UNIVERSAL DATA UPDATER                                   */
/* ======================================================== */
void UpdateMainIOBData(uint8_t action, const uint8_t *data)
{
    if (data == NULL)
    {
        return;
    }

    k_mutex_lock(&shadow_memory_mutex, K_FOREVER);

    if (u8_ProductID == IAM_TRIAD)
    {
        switch (action)
        {
            /* --- ADC TASK SENSORS (2 bytes) --- */
            case SENSOR_COLLECTOR_TEMP:
                memcpy(MainIOBShadowMemory.TRIAD.ADC_Sensors.CollectorTemp.raw, data, 2);
                MainIOBShadowMemory.TRIAD.ADC_Sensors.Last_Update_ms = k_uptime_get_32();
                break;

            case SENSOR_AMBIENT_TEMPERATURE:
                memcpy(MainIOBShadowMemory.TRIAD.ADC_Sensors.AmbientTemp.raw, data, 2);
                MainIOBShadowMemory.TRIAD.ADC_Sensors.Last_Update_ms = k_uptime_get_32();
                break;

            case SENSOR_LEXSOL_TEMP:
                memcpy(MainIOBShadowMemory.TRIAD.ADC_Sensors.LexsolTemp.raw, data, 2);
                MainIOBShadowMemory.TRIAD.ADC_Sensors.Last_Update_ms = k_uptime_get_32();
                break;

            case SENSOR_FIRST_STAGE_HEAT_EXCHANGER_OUTLET_TEMP:
                memcpy(MainIOBShadowMemory.TRIAD.ADC_Sensors.FirstStageHxOutletTemp.raw, data, 2);
                MainIOBShadowMemory.TRIAD.ADC_Sensors.Last_Update_ms = k_uptime_get_32();
                break;

            case SENSOR_SECOND_STAGE_HEAT_EXCHANGER_OUTLET_TEMP:
                memcpy(MainIOBShadowMemory.TRIAD.ADC_Sensors.SecondStageHxOutletTemp.raw, data, 2);
                MainIOBShadowMemory.TRIAD.ADC_Sensors.Last_Update_ms = k_uptime_get_32();
                break;

            case SENSOR_FIRST_STAGE_SUCTION_TEMP:
                memcpy(MainIOBShadowMemory.TRIAD.ADC_Sensors.FirstStageSuctionTemp.raw, data, 2);
                MainIOBShadowMemory.TRIAD.ADC_Sensors.Last_Update_ms = k_uptime_get_32();
                break;

            case SENSOR_SECOND_STAGE_SUCTION_TEMP:
                memcpy(MainIOBShadowMemory.TRIAD.ADC_Sensors.SecondStageSuctionTemp.raw, data, 2);
                MainIOBShadowMemory.TRIAD.ADC_Sensors.Last_Update_ms = k_uptime_get_32();
                break;

            case SENSOR_SHELF1_TEMP:
                memcpy(MainIOBShadowMemory.TRIAD.ADC_Sensors.ShelfTemp.raw, data, 2);
                MainIOBShadowMemory.TRIAD.ADC_Sensors.Last_Update_ms = k_uptime_get_32();
                break;

            case SENSOR_SHELF1_SAMPLE_TEMP:
                memcpy(MainIOBShadowMemory.TRIAD.ADC_Sensors.SampleTemp.raw, data, 2);
                MainIOBShadowMemory.TRIAD.ADC_Sensors.Last_Update_ms = k_uptime_get_32();
                break;

            /* --- VACUUM TASK SENSORS (2 bytes per ICD) --- */
            case SENSOR_SYSTEM_VAC:
                memcpy(MainIOBShadowMemory.TRIAD.Vacuum_Sensors.SystemVacuum.raw, data, 2);
                MainIOBShadowMemory.TRIAD.Vacuum_Sensors.Last_Update_ms = k_uptime_get_32();
                break;

            case SENSOR_SAMPLE_VAC1:
            case SENSOR_SAMPLE_VAC2:
            case SENSOR_SAMPLE_VAC3:
            case SENSOR_SAMPLE_VAC4:
            case SENSOR_SAMPLE_VAC5:
            case SENSOR_SAMPLE_VAC6:
            {
                uint8_t idx = action - SENSOR_SAMPLE_VAC1;
                memcpy(MainIOBShadowMemory.TRIAD.Vacuum_Sensors.SampleVacuum[idx].raw, data, 2);
                MainIOBShadowMemory.TRIAD.Vacuum_Sensors.Last_Update_ms = k_uptime_get_32();
                break;
            }

            case SENSOR_VACUUM_END_POINT_ADDON1:
            case SENSOR_VACUUM_END_POINT_ADDON2:
            {
                uint8_t idx = action - SENSOR_VACUUM_END_POINT_ADDON1;
                memcpy(MainIOBShadowMemory.TRIAD.Vacuum_Sensors.EndPointAddonVacuum[idx].raw, data, 2);
                MainIOBShadowMemory.TRIAD.Vacuum_Sensors.Last_Update_ms = k_uptime_get_32();
                break;
            }

            case SENSOR_HARDWARE_DETECTION_IOB:
                memcpy(&MainIOBShadowMemory.TRIAD.Digital_Sensors.HardwareDetect, data, sizeof(fd_hw_detect_iob_payload_t));
                MainIOBShadowMemory.TRIAD.Digital_Sensors.Last_Update_ms = k_uptime_get_32();
                break;

            default:
                LOG_WRN("UpdateMainIOBData: Unknown TRIAD action 0x%02X", action);
                break;
        }
    }
    else if (u8_ProductID == IAM_FD)
    {
        switch (action)
        {
            /* --- ADC TASK SENSORS (2 bytes) --- */
            case SENSOR_COLLECTOR_TEMP:
                memcpy(MainIOBShadowMemory.FD.ADC_Sensors.CollectorTemp.raw, data, 2);
                MainIOBShadowMemory.FD.ADC_Sensors.Last_Update_ms = k_uptime_get_32();
                break;

            case SENSOR_AMBIENT_TEMPERATURE:
                memcpy(MainIOBShadowMemory.FD.ADC_Sensors.AmbientTemp.raw, data, 2);
                MainIOBShadowMemory.FD.ADC_Sensors.Last_Update_ms = k_uptime_get_32();
                break;

            case SENSOR_FIRST_STAGE_HEAT_EXCHANGER_OUTLET_TEMP:
                memcpy(MainIOBShadowMemory.FD.ADC_Sensors.FirstStageHxOutletTemp.raw, data, 2);
                MainIOBShadowMemory.FD.ADC_Sensors.Last_Update_ms = k_uptime_get_32();
                break;

            case SENSOR_SECOND_STAGE_HEAT_EXCHANGER_OUTLET_TEMP:
                memcpy(MainIOBShadowMemory.FD.ADC_Sensors.SecondStageHxOutletTemp.raw, data, 2);
                MainIOBShadowMemory.FD.ADC_Sensors.Last_Update_ms = k_uptime_get_32();
                break;

            case SENSOR_FIRST_STAGE_SUCTION_TEMP:
                memcpy(MainIOBShadowMemory.FD.ADC_Sensors.FirstStageSuctionTemp.raw, data, 2);
                MainIOBShadowMemory.FD.ADC_Sensors.Last_Update_ms = k_uptime_get_32();
                break;

            case SENSOR_SECOND_STAGE_SUCTION_TEMP:
                memcpy(MainIOBShadowMemory.FD.ADC_Sensors.SecondStageSuctionTemp.raw, data, 2);
                MainIOBShadowMemory.FD.ADC_Sensors.Last_Update_ms = k_uptime_get_32();
                break;

            case SENSOR_SHELL_FREEZER_TEMP_SENSOR:
                memcpy(MainIOBShadowMemory.FD.ADC_Sensors.ShellFreezerTemp.raw, data, 2);
                MainIOBShadowMemory.FD.ADC_Sensors.Last_Update_ms = k_uptime_get_32();
                break;

            case SENSOR_MINI_CHAMBER_TEMP:
                memcpy(MainIOBShadowMemory.FD.ADC_Sensors.MiniChamberTemp.raw, data, 2);
                MainIOBShadowMemory.FD.ADC_Sensors.Last_Update_ms = k_uptime_get_32();
                break;

            /* --- VACUUM TASK SENSORS (2 bytes per ICD) --- */
            case SENSOR_SYSTEM_VAC:
                memcpy(MainIOBShadowMemory.FD.Vacuum_Sensors.SystemVacuum.raw, data, 2);
                MainIOBShadowMemory.FD.Vacuum_Sensors.Last_Update_ms = k_uptime_get_32();
                break;

            case SENSOR_SAMPLE_VAC1:
            case SENSOR_SAMPLE_VAC2:
            case SENSOR_SAMPLE_VAC3:
            case SENSOR_SAMPLE_VAC4:
            case SENSOR_SAMPLE_VAC5:
            case SENSOR_SAMPLE_VAC6:
            {
                uint8_t idx = action - SENSOR_SAMPLE_VAC1;
                memcpy(MainIOBShadowMemory.FD.Vacuum_Sensors.SampleVacuum[idx].raw, data, 2);
                MainIOBShadowMemory.FD.Vacuum_Sensors.Last_Update_ms = k_uptime_get_32();
                break;
            }

            case SENSOR_VACUUM_END_POINT_ADDON1:
            case SENSOR_VACUUM_END_POINT_ADDON2:
            {
                uint8_t idx = action - SENSOR_VACUUM_END_POINT_ADDON1;
                memcpy(MainIOBShadowMemory.FD.Vacuum_Sensors.EndPointAddonVacuum[idx].raw, data, 2);
                MainIOBShadowMemory.FD.Vacuum_Sensors.Last_Update_ms = k_uptime_get_32();
                break;
            }

            case SENSOR_HARDWARE_DETECTION_IOB:
                memcpy(&MainIOBShadowMemory.FD.Digital_Sensors.HardwareDetect, data, sizeof(fd_hw_detect_iob_payload_t));
                MainIOBShadowMemory.FD.Digital_Sensors.Last_Update_ms = k_uptime_get_32();
                break;

            default:
                LOG_WRN("UpdateMainIOBData: Unknown FD action 0x%02X", action);
                break;
        }
    }

    k_mutex_unlock(&shadow_memory_mutex);
}
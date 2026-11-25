/* The structures are updated via communication handler */
#include "accessory_iob_data.h"
#include "product_type.h"
#include "protocol_defs.h"
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(accessory_iob_data, LOG_LEVEL_INF);

AccessoryShadowMemory_t AccessoryShadowMemory;

void AccessoryData_Init(void)
{
    memset(&AccessoryShadowMemory, 0, sizeof(AccessoryShadowMemory));
}

void UpdateAccessoryData(uint8_t action,
                         const uint8_t *data,
                         size_t data_len)
{
    if (data == NULL)
    {
        return;
    }

    switch (action)
    {
        case SENSOR_SHELF1_TEMP:
        case SENSOR_SHELF2_TEMP:
        case SENSOR_SHELF3_TEMP:
        case SENSOR_SHELF4_TEMP:
        case SENSOR_SHELF5_TEMP:
            if (data_len >= 2)
            {
                uint8_t idx = action - SENSOR_SHELF1_TEMP;
                memcpy(AccessoryShadowMemory.ShelfTemp[idx].raw, data, 2);
                AccessoryShadowMemory.Last_Update_ms = k_uptime_get_32();
            }
            break;

        case SENSOR_SHELF1_SAMPLE_TEMP:
        case SENSOR_SHELF2_SAMPLE_TEMP:
        case SENSOR_SHELF3_SAMPLE_TEMP:
        case SENSOR_SHELF4_SAMPLE_TEMP:
        case SENSOR_SHELF5_SAMPLE_TEMP:
            if (data_len >= 2)
            {
                uint8_t idx = action - SENSOR_SHELF1_SAMPLE_TEMP;
                memcpy(AccessoryShadowMemory.SampleTemp[idx].raw, data, 2);
                AccessoryShadowMemory.Last_Update_ms = k_uptime_get_32();
            }
            break;

        case SENSOR_LEXSOL_TEMP:
            if (data_len >= 2)
            {
                memcpy(AccessoryShadowMemory.LexsolTemp.raw, data, 2);
                AccessoryShadowMemory.Last_Update_ms = k_uptime_get_32();
            }
            break;

        case SENSOR_STOPPERING_TRAY_VACUUM:
        case SENSOR_SYSTEM_VAC:
            if (data_len >= 2)
            {
                memcpy(AccessoryShadowMemory.DryerVacuum.raw, data, 2);
                AccessoryShadowMemory.Last_Update_ms = k_uptime_get_32();
            }
            break;

        case SENSOR_DRYING_ACCESSORY_AC_POWER:
            if (data_len >= 1)
            {
                AccessoryShadowMemory.AcPowerOn = data[0];
                AccessoryShadowMemory.Last_Update_ms = k_uptime_get_32();
            }
            break;

        case SENSOR_HARDWARE_SENSOR_DETECTION_ACCY:
            if (data_len >= sizeof(fd_hw_detect_accy_payload_t))
            {
                memcpy(&AccessoryShadowMemory.HardwareDetect, data, sizeof(fd_hw_detect_accy_payload_t));
                AccessoryShadowMemory.Last_Update_ms = k_uptime_get_32();
            }
            break;

        default:
            LOG_WRN("Unhandled accessory sensor action: 0x%02X", action);
            break;
    }
}

void MemoryAllocation(void)
{
    memset(&AccessoryShadowMemory, 0, sizeof(AccessoryShadowMemory));

    for (int i = 0; i < 5; i++)
    {
        AccessoryShadowMemory.ShelfTemp[i].temperature  = 0x7FFF;
        AccessoryShadowMemory.SampleTemp[i].temperature = 0x7FFF;
    }

    AccessoryShadowMemory.LexsolTemp.temperature       = 0x7FFF;
    AccessoryShadowMemory.DryerVacuum.state.vacuum     = 0x7FFF;
    AccessoryShadowMemory.DoorSwitch                   = SWITCH_OPEN;
}
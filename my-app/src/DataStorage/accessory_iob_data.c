/*The structures are updated via communiocation handler*/
#include "accessory_iob_data.h"
#include "product_type.h"
#include "protocol_defs.h"
#include <string.h>
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
    switch (action)
    {
        

        case SENSOR_SYSTEM_VAC:
            // This is for ST
            /* Update SystemVacuum */
            break;
        case SENSOR_SHELF1_TEMP:
            memcpy(AccessoryShadowMemory.BT.ShelfTemp[0].raw,  data, 2);
            break;

        default:
            LOG_INF("Unknown sensor action: 0x%02X", action);
            break;
    }
}

void MemoryAllocation(void)
{
    if (u8_AccessoryID == IAM_BT)
    {
        /*
         * BT:
         * 5 Sample Temperature
         * 5 Shelf Temperature
         * 1 Door Switch
         */
    }

    if (u8_AccessoryID == IAM_ST)
    {
        /*
         * ST:
         * 1 System Vacuum
         * 3 Shelf Temperature
         * 1 Lexsol Temperature
         */
    }
}
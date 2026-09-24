#include <zephyr/kernel.h>
#include "task_telemetry.h"
#include "ProductType/product_type.h"
#include "DataStorage/accessory_iob_data.h"
#include "communication/sensor_handler.h"
#include "communication/protocol_defs.h"

int telemetry_task_init(void) 
{
    // Nothing to initialize for telemetry
    return 0; 
}

void telemetry_task_run(void)
{
    // Wait 1000 milliseconds between streaming bursts
    k_msleep(1000);

    /* --- PUSH MAIN IOB DATA --- */
    if (u8_ProductID == IAM_TRIAD) 
    {
        PushSensorData(SENSOR_COLLECTOR_TEMP);
        k_msleep(20); // 20ms wait ensures it clears even at 9600 baud!
        PushSensorData(SENSOR_SHELF1_TEMP);
        k_msleep(20); 
        PushSensorData(SENSOR_SAMPLE1_TEMP);
        k_msleep(20); 
        PushSensorData(SENSOR_SHELL_DRY_LEXSOL);
        k_msleep(20); 
        PushSensorData(SENSOR_SYSTEM_VAC);
        k_msleep(20); 
        PushSensorData(SENSOR_TILT_SW);
    } 
    else if (u8_ProductID == IAM_FD) 
    {
        PushSensorData(SENSOR_COLLECTOR_TEMP);
        k_msleep(20);
        PushSensorData(SENSOR_MOISTURE);
        k_msleep(20);
        PushSensorData(SENSOR_SYSTEM_VAC);
        // ... add other FD Action IDs here
    }

    /* --- PUSH ACCESSORY IOB DATA --- */
    if (u8_AccessoryID == IAM_BT) 
    {
        // ... add BT Action IDs here
    } 
    else if (u8_AccessoryID == IAM_ST) 
    {
        // ... add ST Action IDs here
    }
}
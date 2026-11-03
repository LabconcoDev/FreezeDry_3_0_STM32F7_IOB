#include "sensor_handler.h"
#include "protocol_defs.h"
#include "DataStorage/accessory_iob_data.h"
#include "DataStorage/main_iob_data.h"
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <string.h>

LOG_MODULE_REGISTER(sensor_handler, LOG_LEVEL_INF);

/* External link to the transmit function */
extern void fd_protocol_send_dma(const uint8_t *data, size_t len);
void PushSensorData(uint8_t action);
void pushMoistureSensor(void);
void pushSystemVacuum(void);
void pushSampleVacuum1(void);
void pushCollectorTemp(void);
void pushSampleVacuum2(void);
void pushSampleVacuum3(void);
void pushSampleVacuum4(void);
void pushSampleVacuum5(void);
void pushSampleVacuum6(void);
// void pushShellDryChamberTemp(void);
void PushShelf1Temp(void);
void PushSample1Temp(void);
void PushTiltSwitch(void);
extern struct k_mutex shadow_memory_mutex;
void fd_sensor_handler_process(uint8_t cls, uint8_t action, const uint8_t *data, size_t data_len) 
{
    
    
    switch (cls)
    {
        case CLASS_WRITE:
            // Invalid message.
            LOG_INF("Msg_Type Sensor Cannot have class Write");
            return;
        case CLASS_PUSH:
            // store and update sensor data in my memory
            UpdateAccessoryData(action, data, data_len);
            break;
        case CLASS_READ:
            // Push Data to APP.
            PushSensorData(action);
    }
}
    
void PushSensorData(uint8_t action)
{
    // The below would be FD/Triad Messages
    // Read Requests of Accessory messages will be handled separately
    switch (action) 
    {
        case SENSOR_MOISTURE:
            pushMoistureSensor();
            break;
        case SENSOR_COLLECTOR_TEMP:
            pushCollectorTemp();
            break;
        case SENSOR_SYSTEM_VAC :
        // 4 bytes per sensor
            pushSystemVacuum();
            break;
        case SENSOR_SAMPLE_VAC1:
        // 4 bytes per sensor
            pushSampleVacuum1();
            break;
        case SENSOR_SAMPLE_VAC2:
        // 4 bytes per sensor
            pushSampleVacuum2();
            break;
        case SENSOR_SAMPLE_VAC3:
        // 4 bytes per sensor
            pushSampleVacuum3();
            break;
        case SENSOR_SAMPLE_VAC4:
        // 4 bytes per sensor
            pushSampleVacuum4();
            break;
        case SENSOR_SAMPLE_VAC5:
        // 4 bytes per sensor
            pushSampleVacuum5();
            break;
        case SENSOR_SAMPLE_VAC6:
        // 4 bytes per sensor
            pushSampleVacuum6();
            break;
        case SENSOR_SHELL_DRY_LEXSOL:
            // pushShellDryChamberTemp();
            break;
        case SENSOR_SHELF1_TEMP:
            PushShelf1Temp();
            break;
        case SENSOR_SAMPLE1_TEMP:
            PushSample1Temp();
            break;
        case SENSOR_TILT_SW:
            PushTiltSwitch();
            break;
        

        default:
            // Error Code
            break;
    }


}   


static void push_sensor_data(uint8_t action,
                             const uint8_t *data,
                             size_t data_len)
{
    uint8_t tx_buf[32];

    size_t frame_len = fd_serializer_build_frame(
        tx_buf,
        sizeof(tx_buf),
        CLASS_PUSH,
        TYPE_SENSORS,
        action,
        data,
        data_len);

    if (frame_len > 0)
    {
        fd_protocol_send_dma(tx_buf, frame_len);
    }
    else
    {
        LOG_ERR("Failed to serialize PUSH packet");
    }
}



void pushCollectorTemp(void)
{
    // Default to sending an Error code (0x7FFF) in case data is stale
    uint8_t out_data[2] = {0xFF, 0x7F}; 
    k_mutex_lock(&shadow_memory_mutex, K_FOREVER);
    
    if (u8_ProductID == IAM_TRIAD) 
    {
        uint32_t age = k_uptime_get_32() - MainIOBShadowMemory.TRIAD.ADC_Sensors.Last_Update_ms;
        if (age < 5000) 
        {
            memcpy(out_data, MainIOBShadowMemory.TRIAD.ADC_Sensors.CollectorTemp.raw, 2);
        }
        else
        {

            // Data is stale.
        }
    } 
    else if (u8_ProductID == IAM_FD) 
    {
        uint32_t age = k_uptime_get_32() - MainIOBShadowMemory.FD.ADC_Sensors.Last_Update_ms;
        if (age < 5000) 
        {
            memcpy(out_data, MainIOBShadowMemory.FD.ADC_Sensors.CollectorTemp.raw, 2);
        }
        else
        {
            // Data is stale
        }
    }
    k_mutex_unlock(&shadow_memory_mutex);
    push_sensor_data(SENSOR_COLLECTOR_TEMP, out_data, 2);
       
}





void pushMoistureSensor(void)
{
    uint8_t current_state = 0x01; // This to be modified to access from actual sensor data type
    push_sensor_data( SENSOR_MOISTURE,  &current_state,  sizeof(current_state));
}
void pushSystemVacuum(void)
{
    uint8_t out_data[4] = {0xFF, 0x7F, 0x00, 0x00}; 

    k_mutex_lock(&shadow_memory_mutex, K_FOREVER);
    
    if (u8_ProductID == IAM_TRIAD) 
    {
        uint32_t age = k_uptime_get_32() - MainIOBShadowMemory.TRIAD.Vacuum_Sensors.Last_Update_ms;
        if (age < 5000) {
            memcpy(out_data, MainIOBShadowMemory.TRIAD.Vacuum_Sensors.SystemVacuum.raw, 4);
        }
    } 
    else if (u8_ProductID == IAM_FD) 
    {
        uint32_t age = k_uptime_get_32() - MainIOBShadowMemory.FD.Vacuum_Sensors.Last_Update_ms;
        if (age < 5000) {
            memcpy(out_data, MainIOBShadowMemory.FD.Vacuum_Sensors.SystemVacuum.raw, 4);
        }
    }

    k_mutex_unlock(&shadow_memory_mutex);

    push_sensor_data(SENSOR_SYSTEM_VAC, out_data, 4);
}
void pushSampleVacuum1(void)
{
    uint8_t current_state = 0x01; // This to be modified to access from actual sensor data type
    push_sensor_data(SENSOR_SAMPLE_VAC1,&current_state, sizeof(current_state) );
}

void pushSampleVacuum2(void)
{
    uint8_t current_state = 0x01; // This to be modified to access from actual sensor data type
    push_sensor_data( SENSOR_SAMPLE_VAC2, &current_state,sizeof(current_state));
}
void pushSampleVacuum3(void)
{
    uint8_t current_state = 0x01; // This to be modified to access from actual sensor data type
    push_sensor_data( SENSOR_SAMPLE_VAC3, &current_state,sizeof(current_state));
}
void pushSampleVacuum4(void)
{
    uint8_t current_state = 0x01; // This to be modified to access from actual sensor data type
    push_sensor_data( SENSOR_SAMPLE_VAC4, &current_state,sizeof(current_state));
}
void pushSampleVacuum5(void)
{
    uint8_t current_state = 0x01; // This to be modified to access from actual sensor data type
    push_sensor_data( SENSOR_SAMPLE_VAC5, &current_state,sizeof(current_state));
}
void pushSampleVacuum6(void)
{
    uint8_t current_state = 0x01; // This to be modified to access from actual sensor data type
    push_sensor_data( SENSOR_SAMPLE_VAC6, &current_state,sizeof(current_state));
}

// void pushShellDryChamberTemp(void)
// {
//     uint8_t out_data[2] = {0xFF, 0x7F}; 

//     k_mutex_lock(&shadow_memory_mutex, K_FOREVER);
    
//     if (u8_ProductID == IAM_TRIAD) 
//     {
//         uint32_t age = k_uptime_get_32() - MainIOBShadowMemory.TRIAD.ADC_Sensors.Last_Update_ms;
//         if (age < 5000) {
//             memcpy(out_data, MainIOBShadowMemory.TRIAD.ADC_Sensors.ShellDryChamberTemp.raw, 2);
//         }
//     }
//     else if (u8_ProductID == IAM_FD) 
//     {
//         uint32_t age = k_uptime_get_32() - MainIOBShadowMemory.FD.ADC_Sensors.Last_Update_ms;
//         if (age < 5000) {
//             memcpy(out_data, MainIOBShadowMemory.FD.ADC_Sensors.ShellDryChamberTemp.raw, 2);
//         }
//     }

//     k_mutex_unlock(&shadow_memory_mutex);

//     push_sensor_data(SENSOR_SHELL_DRY_LEXSOL, out_data, 2);
// }

void PushShelf1Temp(void)
{
    uint8_t out_data[2] = {0xFF, 0x7F}; 

    k_mutex_lock(&shadow_memory_mutex, K_FOREVER);
    
    if (u8_ProductID == IAM_TRIAD) 
    {
        uint32_t age = k_uptime_get_32() - MainIOBShadowMemory.TRIAD.ADC_Sensors.Last_Update_ms;
        if (age < 5000) {
            memcpy(out_data, MainIOBShadowMemory.TRIAD.ADC_Sensors.ShelfTemp.raw, 2);
        }
    }

    k_mutex_unlock(&shadow_memory_mutex);

    push_sensor_data(SENSOR_SHELF1_TEMP, out_data, 2);
}

void PushSample1Temp(void)
{
    uint8_t out_data[2] = {0xFF, 0x7F}; 

    k_mutex_lock(&shadow_memory_mutex, K_FOREVER);
    
    if (u8_ProductID == IAM_TRIAD) 
    {
        uint32_t age = k_uptime_get_32() - MainIOBShadowMemory.TRIAD.ADC_Sensors.Last_Update_ms;
        if (age < 5000) {
            memcpy(out_data, MainIOBShadowMemory.TRIAD.ADC_Sensors.SampleTemp.raw, 2);
        }
    }

    k_mutex_unlock(&shadow_memory_mutex);

    push_sensor_data(SENSOR_SAMPLE1_TEMP, out_data, 2);
}

void PushTiltSwitch(void)
{
    uint8_t out_data[1] = {0x00}; 

    k_mutex_lock(&shadow_memory_mutex, K_FOREVER);
    
    if (u8_ProductID == IAM_TRIAD) 
    {
        uint32_t age = k_uptime_get_32() - MainIOBShadowMemory.TRIAD.Digital_Sensors.Last_Update_ms;
        if (age < 5000) {
            out_data[0] = MainIOBShadowMemory.TRIAD.Digital_Sensors.TiltSwitch;
        }
    }

    k_mutex_unlock(&shadow_memory_mutex);

    push_sensor_data(SENSOR_TILT_SW, out_data, 1);
}
#ifndef MAIN_IOB_DATA_H
#define MAIN_IOB_DATA_H

#include <stdint.h>
#include "sensor_types.h"

void UpdateMainIOBData(uint8_t action, const uint8_t *data);

typedef struct
{
    /* Group 1: All temperatures updated by your local ADC Task */
    struct 
    {
        uint32_t            Last_Update_ms;        // Timestamp for this group
        TemperatureStatus_t CollectorTemp;
        TemperatureStatus_t ShellDryChamberTemp;
    } ADC_Sensors;

    /* Group 2: All vacuums updated by your Vacuum Task */
    struct 
    {
        uint32_t            Last_Update_ms;        // Timestamp for this group
        VacuumStatus_t      SystemVacuum;
        VacuumStatus_t      SampleVacuum[6];       // Array of 6 vacuums
    } Vacuum_Sensors;

    /* Group 3: Digital inputs updated by your GPIO Task */
    struct 
    {
        uint32_t            Last_Update_ms;        // Timestamp for this group
        MoistureStatus_t    Moisture;
    } Digital_Sensors;

} FD_SensorData_t;

typedef struct
{
    /* Group 1: All sensors updated by your local ADC Task */
    struct 
    {
        uint32_t            Last_Update_ms;        // Timestamp for this group
        TemperatureStatus_t CollectorTemp;
        TemperatureStatus_t ShellDryChamberTemp;
        TemperatureStatus_t ShelfTemp;
        TemperatureStatus_t SampleTemp;
    } ADC_Sensors;
    /* Group 2: All sensors updated by your Vacuum Task */
    struct 
    {
        uint32_t            Last_Update_ms;        // Timestamp for this group
        VacuumStatus_t      SystemVacuum;
    } Vacuum_Sensors;
    /* Group 3: All sensors updated by your GPIO Task */
    struct 
    {
        uint32_t            Last_Update_ms;        // Timestamp for this group
        SwitchStatus_t      TiltSwitch;
    } Digital_Sensors;
    
} TRIAD_SensorData_t;


typedef union
{
    FD_SensorData_t    FD;
    TRIAD_SensorData_t TRIAD;

} MainIOBShadowMemory_u;


extern MainIOBShadowMemory_u MainIOBShadowMemory;
void MainIOBMemoryAllocation(void);
void UpdateMainIOBData(uint8_t action, const uint8_t *data);

#endif
#ifndef MAIN_IOB_DATA_H
#define MAIN_IOB_DATA_H

#include <stdint.h>
#include "sensor_types.h"
#include <stdbool.h>   
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






typedef struct {
    uint8_t  segment_no;
    uint8_t  mode;                 /* 0=off, 1=manual, 2=auto, 3=program */
    uint8_t  phase;                /* 0=Hold, 1=Ramp */
    int16_t  shelf_target_dC;      /* In 0.1 C */
    int16_t  ramp_rate;            /* In 0.01 C/min */
    uint16_t vac_target_mbar;     /* In mTorr / uBar */
    uint16_t hold_time_total_min;
    uint16_t hold_time_remaining_min;
    uint8_t  control_flags;
    bool     is_running;
} TRIAD_ProgramState_t;



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


    TRIAD_ProgramState_t    Program_State;
    
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
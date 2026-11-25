#ifndef MAIN_IOB_DATA_H
#define MAIN_IOB_DATA_H

#include <stdint.h>
#include <stdbool.h>
#include "sensor_types.h"
#include "communication/protocol_defs.h"

typedef struct
{
    /* Group 1: All temperatures updated by local ADC Task */
    struct
    {
        uint32_t            Last_Update_ms;
        TemperatureStatus_t CollectorTemp;
        TemperatureStatus_t AmbientTemp;
        TemperatureStatus_t FirstStageHxOutletTemp;
        TemperatureStatus_t SecondStageHxOutletTemp;
        TemperatureStatus_t FirstStageSuctionTemp;
        TemperatureStatus_t SecondStageSuctionTemp;
        TemperatureStatus_t ShellFreezerTemp;
        TemperatureStatus_t MiniChamberTemp;
    } ADC_Sensors;

    /* Group 2: All vacuums updated by Vacuum Task */
    struct
    {
        uint32_t            Last_Update_ms;
        VacuumStatus_t      SystemVacuum;
        VacuumStatus_t      SampleVacuum[6];
        VacuumStatus_t      EndPointAddonVacuum[2];
    } Vacuum_Sensors;

    /* Group 3: Digital inputs & hardware detection */
    struct
    {
        uint32_t                   Last_Update_ms;
        MoistureStatus_t           Moisture;
        fd_hw_detect_iob_payload_t HardwareDetect;
    } Digital_Sensors;

} FD_SensorData_t;

typedef struct
{
    uint8_t  segment_no;
    uint8_t  mode;                 /* 0=off, 1=manual, 2=auto, 3=program */
    uint8_t  phase;                /* 0=Ramp, 1=Hold (matches ICD) */
    int16_t  shelf_target_dC;      /* In 0.1 C */
    uint16_t ramp_rate;            /* In 0.01 C/min */
    uint16_t vac_target_mbar;      /* In mbar * 1000 */
    uint16_t hold_time_total_min;
    uint16_t hold_time_remaining_min;
    uint8_t  control_flags;
    bool     is_running;
} TRIAD_ProgramState_t;

typedef struct
{
    /* Group 1: All temperatures updated by local ADC Task */
    struct
    {
        uint32_t            Last_Update_ms;
        TemperatureStatus_t CollectorTemp;
        TemperatureStatus_t AmbientTemp;
        TemperatureStatus_t LexsolTemp;
        TemperatureStatus_t FirstStageHxOutletTemp;
        TemperatureStatus_t SecondStageHxOutletTemp;
        TemperatureStatus_t FirstStageSuctionTemp;
        TemperatureStatus_t SecondStageSuctionTemp;
        TemperatureStatus_t ShelfTemp;
        TemperatureStatus_t SampleTemp;
    } ADC_Sensors;

    /* Group 2: All vacuums updated by Vacuum Task */
    struct
    {
        uint32_t            Last_Update_ms;
        VacuumStatus_t      SystemVacuum;
        VacuumStatus_t      SampleVacuum[6];
        VacuumStatus_t      EndPointAddonVacuum[2];
    } Vacuum_Sensors;

    /* Group 3: Digital inputs & hardware detection */
    struct
    {
        uint32_t                   Last_Update_ms;
        SwitchStatus_t             TiltSwitch;
        fd_hw_detect_iob_payload_t HardwareDetect;
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

#endif /* MAIN_IOB_DATA_H */
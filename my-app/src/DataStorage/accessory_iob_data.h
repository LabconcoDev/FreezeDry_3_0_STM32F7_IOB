#ifndef ACCESSORY_IOB_DATA_H
#define ACCESSORY_IOB_DATA_H

#include <stdint.h>
#include <stddef.h>

#include "sensor_types.h"

extern uint8_t u8_ProductID;
extern uint8_t u8_AccessoryID;
void AccessoryData_Init(void);
void UpdateAccessoryData(uint8_t action,
                         const uint8_t *data,
                         size_t data_len);


/* ---------------- BT ---------------- */

typedef struct
{
    TemperatureStatus_t SampleTemp[5];
    TemperatureStatus_t ShelfTemp[5];
    SwitchStatus_t      DoorSwitch;

} BT_AccessoryData_t;


/* ---------------- ST ---------------- */

typedef struct
{
    VacuumStatus_t      SystemVacuum;
    TemperatureStatus_t ShelfTemp[3];
    TemperatureStatus_t LexsolTemp;

} ST_AccessoryData_t;


/* ---------------- Accessory shadow memory ---------------- */

typedef struct
{
    BT_AccessoryData_t BT;
    ST_AccessoryData_t ST;

} AccessoryShadowMemory_t;


/* Global shadow memory */
extern AccessoryShadowMemory_t AccessoryShadowMemory;

void MemoryAllocation(void);

#endif
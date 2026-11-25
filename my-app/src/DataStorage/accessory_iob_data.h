#ifndef ACCESSORY_IOB_DATA_H
#define ACCESSORY_IOB_DATA_H

#include <stdint.h>
#include <stddef.h>
#include "sensor_types.h"
#include "communication/protocol_defs.h"

extern uint8_t u8_ProductID;
extern uint8_t u8_AccessoryID;

void AccessoryData_Init(void);
void UpdateAccessoryData(uint8_t action,
                         const uint8_t *data,
                         size_t data_len);
void MemoryAllocation(void);

/* ---------------- Accessory shadow memory ---------------- */
/* Shared across BT, mBT, HPS, and ST */
typedef struct
{
    uint32_t                    Last_Update_ms;
    TemperatureStatus_t         ShelfTemp[5];
    TemperatureStatus_t         SampleTemp[5];
    TemperatureStatus_t         LexsolTemp;
    VacuumStatus_t              DryerVacuum;
    SwitchStatus_t              DoorSwitch;
    uint8_t                     AcPowerOn;
    fd_hw_detect_accy_payload_t HardwareDetect;
} AccessoryShadowMemory_t;

/* Global shadow memory */
extern AccessoryShadowMemory_t AccessoryShadowMemory;

#endif /* ACCESSORY_IOB_DATA_H */
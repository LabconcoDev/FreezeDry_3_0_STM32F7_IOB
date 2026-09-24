/* sensor_types.h */
#ifndef SENSOR_TYPES_H
#define SENSOR_TYPES_H

#include <stdint.h>

typedef enum {
    MOISTURE_NO  = 0,
    MOISTURE_PRESENT = 1
} MoistureStatus_t;

typedef union {
    uint8_t raw[2];
    int16_t temperature;
} TemperatureStatus_t;

typedef union {
    uint8_t raw[4];
    struct {
        int16_t vacuum;
        uint8_t status;
        uint8_t reserved;
    } state;
} VacuumStatus_t;

typedef enum {
    SWITCH_OPEN   = 0,
    SWITCH_CLOSED = 1
} SwitchStatus_t;

#endif
#ifndef FD_PROTOCOL_DEFS_H_
#define FD_PROTOCOL_DEFS_H_

#include <stdint.h>

#define FD_SOF_L             0x4C // 'L'
#define FD_SOF_C             0x43 // 'C'
#define FD_HEADER_SIZE       3    // 'L', 'C', N
#define FD_CHECKSUM_SIZE     2    // LSB, MSB
#define FD_OVERHEAD_SIZE     (FD_HEADER_SIZE + FD_CHECKSUM_SIZE)
#define FD_MAX_PAYLOAD_SIZE  128

typedef enum {
    CLASS_READ  = 0x01,
    CLASS_WRITE = 0x02,
    CLASS_PUSH  = 0x03,
    CLASS_ACK   = 0x04,
} fd_class_t;

typedef enum {
    TYPE_SENSORS    = 0x01,
    TYPE_CONTROL    = 0x02,
    TYPE_ALERT      = 0x03,
    TYPE_SYSTEM     = 0x04,
    TYPE_SETTINGS   = 0x05,
    TYPE_DIAGNOSTIC = 0x06,
} fd_type_t;

typedef enum {
    CONTROL_REFRIG_RLY              = 0x01,
    CONTROL_VACUUM_RLY              = 0x02,
    CONTROL_PURGE_RLY               = 0x03,
    CONTROL_SHELL_RLY               = 0x04,
    CONTROL_REFRIG_DLY_RLY          = 0x05, // this is second compressor
    CONTROL_VENT_VALVE              = 0x06,
    CONTROL_DEFROST_RELAY           = 0x07,
    CONTROL_MINI_CHAMBER_HTR        = 0x08,
    CONTROL_SHELF_MSTR_HTR_CNTRL    = 0x09,
    CONTROL_SHELF1_HTR              = 0x0A,
    CONTROL_SHELF2_HTR              = 0x0B,
    CONTROL_SHELF3_HTR              = 0x0C,
    CONTROL_SHELF4_HTR              = 0x0D,
    CONTROL_SHELF5_HTR              = 0x0E,
    CONTROL_LEXSOL_PUMP_RLY         = 0x0F,
    CONTROL_LEXSOL_HTR              = 0x10,
    CONTROL_REF_VALVE_RLY           = 0x11,
    CONTROL_TILT_VALVE              = 0x12,
    CONTROL_5V_FAN_DRV              = 0x13,

    /* Add others from ICD as needed... */
} fd_ctrl_action_t;


typedef enum {
    SYSTEM_VERSION              = 0x01,
    SYSTEM_MODEL_DETAILS        = 0x02,
    SYSTEM_DEBUG_LOGS           = 0x03,
    SYSTEM_DATE                 = 0x04,
    SYSTEM_TIME                 = 0x05,
    SYSTEM_IAM_ALIVE            = 0x06,
    SYSTEM_IAM_DONE             = 0x07,
    SYSTEM_PING                 = 0x08,
    SYSTEM_TERMINAL             = 0x09,
    SYSTEM_PRODUCT_TYPE         = 0x0A,
    /* Add others from ICD as needed... */
} fd_system_action_t;



typedef enum {
    SENSOR_COLLECTOR_TEMP                = 0x01,
    SENSOR_MINI_CHAMBER_TEMP             = 0x02,
    SENSOR_SYSTEM_VAC                    = 0x03,
    SENSOR_SAMPLE_VAC1                   = 0x04,
    SENSOR_SAMPLE_VAC2                   = 0x05, 
    SENSOR_SAMPLE_VAC3                   = 0x06,
    SENSOR_SAMPLE_VAC4                   = 0x07,
    SENSOR_SAMPLE_VAC5                   = 0x08,
    SENSOR_SAMPLE_VAC6                   = 0x09,
    SENSOR_MOISTURE                      = 0x0A,
    SENSOR_SHELL_TEMP_SENSOR             = 0x0B,
    SENSOR_SHELF1_TEMP                   = 0x0C,
    SENSOR_SHELF2_TEMP                   = 0x0D,
    SENSOR_SHELF3_TEMP                   = 0x0E,
    SENSOR_SHELF4_TEMP                   = 0x0F,
    SENSOR_SHELF5_TEMP                   = 0x10,
    SENSOR_SAMPLE1_TEMP                  = 0x11,
    SENSOR_SAMPLE2_TEMP                  = 0x12,
    SENSOR_SAMPLE3_TEMP                  = 0x13,
    SENSOR_SAMPLE4_TEMP                  = 0x14,
    SENSOR_SAMPLE5_TEMP                  = 0x15,
    SENSOR_DOOR_SW                       = 0x16,
    SENSOR_TILT_SW                       = 0x17,
    SENSOR_SHELL_DRY_LEXSOL              = 0x18,
    
    /* Add others from ICD as needed... */
} fd_sensor_action_t;




/* Standard Payload Structure */
typedef struct __attribute__((packed)) {
    uint8_t cls;     /* fd_class_t */
    uint8_t type;    /* fd_type_t */
    uint8_t action;
    uint8_t data[];  /* Variable length data */
} fd_payload_t;

typedef struct {
    uint8_t len;
    fd_payload_t *payload;
} fd_packet_t;

#endif /* FD_PROTOCOL_DEFS_H_ */
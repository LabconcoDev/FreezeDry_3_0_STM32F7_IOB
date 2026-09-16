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
    CONTROL_REFRIG_RLY      = 0x01,
    CONTROL_VACUUM_RLY      = 0x02,
    CONTROL_PURGE_RLY       = 0x03,
    CONTROL_SHELL_RLY       = 0x04,
    CONTROL_REFRIG_DLY_RLY  = 0x05,
    CONTROL_VENT_VALVE      = 0x06,
    CONTROL_DEFROST_RELAY   = 0x07,
    CONTROL_MINI_CHAMBER_HTR= 0x08,
    /* Add others from ICD as needed... */
} fd_ctrl_action_t;

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
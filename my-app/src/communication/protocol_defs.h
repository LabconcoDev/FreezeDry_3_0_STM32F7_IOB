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
    CONTROL_REFRIG_RLY          = 0x01,
    CONTROL_VACUUM_RLY          = 0x02,
    CONTROL_PURGE_RLY           = 0x03,
    CONTROL_SHELL_FREEZER_RLY   = 0x04,
    CONTROL_DEFROST_RLY         = 0x05,
    CONTROL_VACUUM_VENT_VALVE   = 0x06,
    CONTROL_MINI_CHAMBER_HEATER = 0x07,
    CONTROL_MANUAL_SHELF_HEAT   = 0x08,
    CONTROL_AUTO_MODE           = 0x09,
    CONTROL_PROGRAM_SEGMENTS    = 0x0A,
    CONTROL_PROGRAM_RUN         = 0x0B,
    CONTROL_PROGRAM_STATUS      = 0x0C,
} fd_ctrl_action_t;

typedef enum {
    SENSOR_COLLECTOR_TEMP                         = 0x01,
    SENSOR_AMBIENT_TEMPERATURE                    = 0x02,
    SENSOR_LEXSOL_TEMP                            = 0x03,
    SENSOR_FIRST_STAGE_HEAT_EXCHANGER_OUTLET_TEMP = 0x04,
    SENSOR_SECOND_STAGE_HEAT_EXCHANGER_OUTLET_TEMP= 0x05,
    SENSOR_FIRST_STAGE_SUCTION_TEMP               = 0x06,
    SENSOR_SECOND_STAGE_SUCTION_TEMP              = 0x07,
    SENSOR_SHELL_FREEZER_TEMP_SENSOR              = 0x08,
    SENSOR_MINI_CHAMBER_TEMP                      = 0x09,
    SENSOR_SHELF1_TEMP                            = 0x0A,
    SENSOR_SHELF2_TEMP                            = 0x0B,
    SENSOR_SHELF3_TEMP                            = 0x0C,
    SENSOR_SHELF4_TEMP                            = 0x0D,
    SENSOR_SHELF5_TEMP                            = 0x0E,
    SENSOR_SHELF1_SAMPLE_TEMP                     = 0x0F,
    SENSOR_SHELF2_SAMPLE_TEMP                     = 0x10,
    SENSOR_SHELF3_SAMPLE_TEMP                     = 0x11,
    SENSOR_SHELF4_SAMPLE_TEMP                     = 0x12,
    SENSOR_SHELF5_SAMPLE_TEMP                     = 0x13,
    SENSOR_SYSTEM_VAC                             = 0x14,
    SENSOR_SAMPLE_VAC1                            = 0x15,
    SENSOR_SAMPLE_VAC2                            = 0x16,
    SENSOR_SAMPLE_VAC3                            = 0x17,
    SENSOR_SAMPLE_VAC4                            = 0x18,
    SENSOR_SAMPLE_VAC5                            = 0x19,
    SENSOR_SAMPLE_VAC6                            = 0x1A,
    SENSOR_STOPPERING_TRAY_VACUUM                 = 0x1B,
    SENSOR_VACUUM_END_POINT_ADDON1                = 0x1C,
    SENSOR_VACUUM_END_POINT_ADDON2                = 0x1D,
    SENSOR_DRYING_ACCESSORY_AC_POWER              = 0x1E,
    SENSOR_HARDWARE_DETECTION_IOB                 = 0x1F,
    SENSOR_HARDWARE_SENSOR_DETECTION_ACCY         = 0x20,
} fd_sensor_action_t;




typedef enum {
    SYSTEM_FIRMWARE_VERSION_IOB          = 0x01,
    SYSTEM_MODEL_DETAILS_IOB             = 0x02,
    SYSTEM_DEBUG_LOG                     = 0x03,
    SYSTEM_DATE                          = 0x04,
    SYSTEM_TIME                          = 0x05,
    SYSTEM_I_AM_ALIVE                    = 0x06,
    SYSTEM_I_AM_DONE                     = 0x07,
    SYSTEM_PING                          = 0x08,
    SYSTEM_TERMINAL                      = 0x09,
    SYSTEM_FIRMWARE_VERSION_ACCY         = 0x0A,
    SYSTEM_MODEL_DETAILS_ACCY            = 0x0B,
    SYSTEM_FIRMWARE_UPDATE_REQUEST_ACCY  = 0x0C,
    SYSTEM_FIRMWARE_UPDATE_RESULT_ACCY   = 0x0D,
    SYSTEM_FIRMWARE_UPDATE_PROGRESS_ACCY = 0x0E,
} fd_system_action_t;




typedef enum {
    /* Ack / Clear */
    ALERT_ACK_BY_USER_CLEAR_ALL                  = 0x01,
    ALERT_CLEAR_SPECIFIC_ALARM                   = 0x02,

    /* Temperature */
    ALERT_SHELF_TEMP_VAR                         = 0x03,
    ALERT_COLLECTOR_TEMP_VAR                     = 0x04,
    ALERT_SHELL_TEMP_VAR                         = 0x05,
    ALERT_LEXSOL_TEMP_VAR                        = 0x06,
    ALERT_LEXSOL_TEMP_SET_POINT_FAIL             = 0x07,
    ALERT_SHELF_TEMP_SET_POINT_TIMEOUT           = 0x08,
    ALERT_TEMP_RANGE_OUT                         = 0x09,
    ALERT_TEMP_PROBE_ERROR                       = 0x0A,

    /* Vacuum */
    ALERT_VACUUM_LEAK                            = 0x0B,
    ALERT_VACUUM_VAR                             = 0x0C,
    ALERT_VACUUM_SET_POINT_TIMEOUT               = 0x0D,
    ALERT_VACUUM_PROBE_ERROR                     = 0x0E,
    ALERT_VACUUM_VAR_SET_POINT_STOPPERING_TRAY   = 0x0F,

    /* Hardware / Safety */
    ALERT_PLATEN_TILT                            = 0x10,
    ALERT_COLLECTOR_MOISTURE                     = 0x11,

    /* End Point Detection */
    ALERT_END_POINT1_ACHIEVED                    = 0x12,
    ALERT_END_POINT2_ACHIEVED                    = 0x13,
    ALERT_END_POINT3_ACHIEVED                    = 0x14,
    ALERT_END_POINT4_ACHIEVED                    = 0x15,
    ALERT_END_POINT5_ACHIEVED                    = 0x16,

    /* Program */
    ALERT_PROGRAM_COMPLETE                       = 0x17,
    ALERT_PROGRAM_SEGMENT_COMPLETE               = 0x18,

    /* Door & Power */
    ALERT_DOOR_STATUS                            = 0x19,
    ALERT_POWER_FAIL_IOB                         = 0x1A,
    ALERT_POWER_FAIL_ACCESSORY                   = 0x1B,
} fd_alert_action_t;

typedef enum 
{
    SETTING_CALIB_TEMP_OFFSET                    = 0x01,
    SETTING_CALIB_VACUUM_OFFSET                  = 0x02,
} fd_settings_action_t;
typedef enum 
{
    DIAGNOSTIC_COMPLETE                          = 0x01,
    DIAGNOSTIC_START                             = 0x02,
    DIAGNOSTIC_STOP                              = 0x03,
} fd_diagnostic_action_t;

/* =========================================================
 * CONTROL PAYLOADS & BITMASKS
 * ========================================================= */
/* CONTROL_VACUUM_RLY (0x02) - 3 bytes (PUSH sends B0 only) */
#define CTRL_VAC_ON_OFF_MASK        0x0FU  /* Bits 3-0: ON(1)/OFF(0) */
#define CTRL_VAC_PROTECT_FLAG       (1U << 4) /* Bit 7-4: vac protect flag (1=set) */

typedef struct __attribute__((packed)) // No padding
{
    uint8_t  flags;             /* B0: Bits 3-0 ON/OFF, Bits 7-4 vac protect */
    uint16_t vacuum_setpoint;   /* B1-B2: mbar x 1000 (LSB, MSB) */
} fd_ctrl_vacuum_payload_t;

/* CONTROL_MINI_CHAMBER_HEATER (0x07) - 3 bytes (PUSH sends B0 only) */
typedef struct __attribute__((packed)) 
{
    uint8_t  on_off;            /* B0: 1=ON, 0=OFF */
    int16_t  target_temp_dC;    /* B1-B2: deg C x 10 (LSB, MSB) */
} fd_ctrl_mini_chamber_payload_t;

/* CONTROL_MANUAL_SHELF_HEAT (0x08) - 3 bytes (PUSH sends B0 only) */
#define CTRL_SHELF_ON_OFF_MASK      0x0FU  /* Bits 3-0: ON(1)/OFF(0) */
#define CTRL_SHELF_MAXCOLD          (1U << 4) /* Bit 4: Maxcold */
#define CTRL_SHELF_IN_FLASK         (1U << 5) /* Bit 5: In Flask */

typedef struct __attribute__((packed)) 
{
    uint8_t  flags;             /* B0: Bits 3-0 ON/OFF, Bit 4 Maxcold, Bit 5 In Flask */
    int16_t  target_temp_dC;    /* B1-B2: deg C x 10 (LSB, MSB) */
} fd_ctrl_manual_shelf_payload_t;

/* Bitmask for B1 Control Flags (CONTROL_PROGRAM_SEGMENTS) */
#define PGM_CTRL_VAC_ON_OFF         (1U << 0)  /* Bit 0: Vac ON/OFF */
#define PGM_CTRL_VAC_PROTECT        (1U << 1)  /* Bit 1: VAC Protect */
#define PGM_CTRL_MAX_COLD           (1U << 2)  /* Bit 2: Max Cold */
#define PGM_CTRL_INFINITE_HOLD      (1U << 3)  /* Bit 3: Infinite Hold */
/* 10-Byte Payload for CONTROL_PROGRAM_SEGMENTS (0x0A) - Little Endian (LSB first) */

typedef struct __attribute__((packed)) 
{
    uint8_t  segment_no;        /* B0: Segment No. */
    uint8_t  control_flags;     /* B1: Control bitmask above */
    int16_t  shelf_temp_dC;     /* B2-B3: Shelf Temp x 10 (LSB, MSB) */
    uint16_t ramp_rate;         /* B4-B5: Ramp Rate x 100 (LSB, MSB) */
    uint16_t vacuum_mbar;       /* B6-B7: Vacuum in mbar x 1000 (LSB, MSB) */
    uint16_t hold_time_mins;    /* B8-B9: Hold time in mins (LSB, MSB) */
} fd_program_segment_payload_t;

/* 4-Byte Payload for CONTROL_PROGRAM_STATUS (0x0C) - Little Endian (LSB first) */
typedef struct __attribute__((packed)) 
{
    uint8_t  segment_no;        /* B0: Currently running segment No. */
    uint16_t time_left_mins;    /* B1-B2: Time left in mins (LSB, MSB) */
    uint8_t  ramp_hold;         /* B3: Ramp (0) / Hold (1) */
} fd_program_status_payload_t;

/* =========================================================
 * SENSOR HARDWARE DETECTION PAYLOADS
 * ========================================================= */
/* SENSOR_HARDWARE_DETECTION_IOB (0x1F) - 3 bytes */
#define HW_OPT_NONE                 0x00
#define HW_OPT_SHELL_FREEZER        0x01
#define HW_OPT_MINI_CHAMBER         0x02
#define HW_VALVE_PURGE              (1U << 0)
#define HW_VALVE_TILT               (1U << 1)
#define HW_VALVE_VENT               (1U << 2)

typedef struct __attribute__((packed)) 
{
    uint8_t optional_hw;        /* B0: 0=None, 1=Shell Freezer, 2=Mini Chamber */
    uint8_t valve_detect;       /* B1: Bit 0 Purge, Bit 1 Tilt, Bit 2 Vent */
    uint8_t epd_sensors;        /* B2: Bits 5-0 active EPD sensors */
} fd_hw_detect_iob_payload_t;

/* SENSOR_HARDWARE_SENSOR_DETECTION_ACCY (0x20) - 2 bytes */
typedef struct __attribute__((packed)) 
{
    uint8_t active_shelves;     /* B0: Bits 5-0 active shelf RTDs */
    uint8_t active_samples;     /* B1: Bits 5-0 active sample RTDs */
} fd_hw_detect_accy_payload_t;


/* =========================================================
 * SYSTEM PAYLOADS
 * ========================================================= */

 typedef struct __attribute__((packed)) 
 {
    char model[4];              /* B0-B3 */
    char special[4];            /* B4-B7 */
    char serial[4];             /* B8-B11 */
} fd_model_details_payload_t;
typedef struct __attribute__((packed)) 
{
    uint8_t day;                /* B0 */
    uint8_t month;              /* B1 */
    uint8_t year;               /* B2: Last 2 digits of 20xx */
} fd_date_payload_t;
typedef struct __attribute__((packed)) 
{
    uint8_t seconds;            /* B0 */
    uint8_t minutes;            /* B1 */
    uint8_t hours;              /* B2 */
} fd_time_payload_t;


/* =========================================================
 * SETTINGS & DIAGNOSTIC PAYLOADS
 * ========================================================= */
typedef enum 
{
    CALIB_TEMP_COLLECTOR    = 1,
    CALIB_TEMP_LEXSOL       = 2,
    CALIB_TEMP_MINI_CHAMBER = 3,
    CALIB_TEMP_SHELL_FREEZER= 4,
    CALIB_TEMP_SAMPLE_1     = 5,  /* 5-9: Sample 1-5 */
    CALIB_TEMP_SHELF_1      = 10, /* 10-14: Shelf 1-5 */
} fd_calib_temp_sensor_t;
typedef enum 
{
    CALIB_VAC_SYSTEM        = 1,
    CALIB_VAC_ST_DRYER      = 2,
    CALIB_VAC_SAMPLE_1      = 3,  /* 3-8: Sample Vacuum 1-6 */
} fd_calib_vac_sensor_t;
typedef struct __attribute__((packed)) 
{
    uint8_t target_sensor;      /* B0: Target sensor ID */
    int16_t offset;             /* B1-B2: Signed offset (LSB, MSB) */
} fd_calib_offset_payload_t;
typedef enum 
{
    DIAG_TEST_DEFROST_RELAY     = 1,
    DIAG_TEST_VENT_VALVE        = 2,
    DIAG_TEST_PREFREEZE_VALVE   = 3,
    DIAG_TEST_VACUUM_PUMP       = 4,
    DIAG_TEST_COMPRESSOR_RELAYS = 5,
    DIAG_TEST_LEXSOL_PUMP       = 6,
    DIAG_TEST_LEXSOL_HEATER     = 7,
    DIAG_TEST_PURGE_VALVE       = 8,
} fd_diag_test_id_t;
typedef struct __attribute__((packed)) 
{
    uint8_t test_id;            /* B0: fd_diag_test_id_t */
    uint8_t pass_fail;          /* B1: 1=Pass, 0=Fail */
} fd_diag_complete_payload_t;

/* =========================================================
 * STANDARD FRAME PAYLOAD & PACKET
 * ========================================================= */
typedef struct __attribute__((packed)) 
{
    uint8_t cls;     /* fd_class_t */
    uint8_t type;    /* fd_type_t */
    uint8_t action;
    uint8_t data[];  /* Variable length data */
} fd_payload_t;
typedef struct 
{
    uint8_t len;
    fd_payload_t *payload;
} fd_packet_t;
#endif /* FD_PROTOCOL_DEFS_H_ */
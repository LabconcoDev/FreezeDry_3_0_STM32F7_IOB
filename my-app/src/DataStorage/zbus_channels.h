#ifndef ZBUS_CHANNELS_H
#define ZBUS_CHANNELS_H

#include <stdint.h>
#include <stdbool.h>
#include <zephyr/zbus/zbus.h>

// Temperature
struct sys_temp_msg
{
    int16_t  collector_dC;          /* 0x01: FD, TRIAD */
    int16_t  ambient_dC;            /* 0x02: FD, TRIAD */
    int16_t  hx_1st_stage_dC;       /* 0x04: FD, TRIAD */
    int16_t  hx_2nd_stage_dC;       /* 0x05: FD, TRIAD */
    int16_t  suction_1st_stage_dC;  /* 0x06: FD, TRIAD */
    int16_t  suction_2nd_stage_dC;  /* 0x07: FD, TRIAD */
    int16_t  shell_or_mini_dC;      /* 0x08/0x09: FD (Shell Freezer or Mini Chamber) */
    int16_t  triad_lexsol_dC;       /* 0x03: TRIAD */
    int16_t  triad_shelf_dC;        /* 0x0A: TRIAD */
    int16_t  triad_sample_dC;       /* 0x0F: TRIAD */
    uint32_t timestamp_ms;
    bool     valid;
};


struct accy_temp_msg
{
    
    int16_t  st_lexsol_dC;  /* 0x03: ST Lexsol Temp */
    int16_t  shelf_dC[5];   /* 0x0A-0x0E: Shelf 1-5 Temp (BT, mBT, HPS, ST) */
    int16_t  sample_dC[5];  /* 0x0F-0x13: Sample 1-5 Temp (BT, mBT, HPS, ST) */  
    uint32_t timestamp_ms;
    bool     valid;
};



// Control Status

typedef enum
{
    CTRL_MODE_OFF     = 0,
    CTRL_MODE_MANUAL  = 1,
    CTRL_MODE_AUTO    = 2,
    CTRL_MODE_PROGRAM = 3,
} ctrl_operating_mode_t;

/* (FD & TRIAD) */
#define CTRL_OUT_REFRIG_1ST        (1U << 0)  /* 0x01: First stage compressor */
#define CTRL_OUT_REFRIG_2ND        (1U << 1)  /* Second stage cascade compressor */
#define CTRL_OUT_VACUUM_PUMP       (1U << 2)  /* 0x02: Vacuum pump relay */
#define CTRL_OUT_DEFROST           (1U << 3)  /* 0x05: Defrost relay */
#define CTRL_OUT_VAC_VENT_VALVE    (1U << 4)  /* 0x06: Vacuum vent valve */
#define CTRL_OUT_TILT_VALVE        (1U << 5)  /* Tilt valve */

/* FD-Only */
#define CTRL_OUT_FD_PURGE_VALVE    (1U << 6)  /* 0x03: Purge valve */
#define CTRL_OUT_FD_SHELL_FREEZER  (1U << 7)  /* 0x04: Shell freezer relay */
#define CTRL_OUT_FD_MINI_HEATER    (1U << 8)  /* 0x07: Mini chamber heater */

/* TRIAD-Only */
#define CTRL_OUT_TRIAD_PREFREEZE   (1U << 9)  /* 0x03: Pre-freeze refrigeration valve */
#define CTRL_OUT_TRIAD_LEXSOL_PUMP (1U << 10) /* 0x04: Lexsol circulation pump */
#define CTRL_OUT_TRIAD_FAN         (1U << 11) /* TRIAD fan */
#define CTRL_OUT_TRIAD_SHELF_HEAT  (1U << 12) /* 0x08: TRIAD shelf heater */

struct sys_control_status_msg
{
    /* Operating Mode & Program State (TRIAD) */
    uint8_t  mode;                    /* ctrl_operating_mode_t */
    uint8_t  program_segment_no;      /* 0x0C: Program segment number */
    uint16_t program_time_left_mins;  /* 0x0C: Program time left (mins) */
    uint8_t  program_ramp_hold;       /* 0x0C: 0 = Ramp, 1 = Hold */

    /* Relay & Valve Data */
    uint16_t output_flags;            /* CTRL_OUT_* bitmask */

    /* Active Setpoints & Flags */
    bool     vacuum_protect_enabled;  /* Common: 0x02 / 0x0A */
    uint16_t vacuum_setpoint_mbar;    /* Common: mbar * 1000 */
    int16_t  mini_chamber_target_dC;  /* FD-Only: deg C * 10 */
    uint8_t  shelf_heat_flags;        /* TRIAD-Only: MaxCold, InFlask */
    int16_t  shelf_target_dC;         /* TRIAD-Only: deg C * 10 */

    /* Metadata */
    uint32_t timestamp_ms;
    bool     valid;
};

/* Command message sent from UART parser -> 1_Control_Thread */
struct ctrl_cmd_msg
{
    uint8_t action;
    uint8_t data[12];
    uint8_t data_len;
};

// control status accessory

struct accy_control_status_msg
{
    /* Manual Shelf Heat (ICD 0x08) */
    bool     shelf_heater_on;         /* Shelf Heater ON/OFF */

    /* Program Data (ICD 0x0B-0x0C) */
    bool     program_running;         /* Program Running Flag */
    uint8_t  program_segment_no;      /* Program Segment Number */
    uint16_t program_time_left_mins;  /* Program Time Left (mins) */
    uint8_t  program_ramp_hold;       /* Program Ramp/Hold (0 = Ramp, 1 = Hold) */

    /* Active Setpoints & Flags (ICD 0x08, 0x0A) */
    int16_t  shelf_target_dC;         /* Shelf Target Temp (deg C * 10) */
    uint8_t  program_control_flags;   /* Vac ON/OFF, Vac Protect, InfHold */
    uint16_t program_vacuum_mbar;     /* Program Vacuum Setpoint (mbar * 1000) */

    /* Meta Data */
    uint32_t timestamp_ms;
    bool     valid;
};


// system vacuum 
struct sys_vac_msg
{
    uint16_t system_vac_mbar;      /* 0x14: System Vacuum (mbar * 1000) */
    uint16_t sample_vac_mbar[6];   /* 0x15-0x1A: Sample Vacuum 1-6 */
    uint16_t epd_addon_vac_1_mbar; /* 0x1C: Vacuum End Point Add-on 1 */
    uint16_t epd_addon_vac_2_mbar; /* 0x1D: Vacuum End Point Add-on 2 */
    uint32_t timestamp_ms;
    bool     valid;
};

ZBUS_CHAN_DECLARE(ZBUS_CHAN_SYSTEM_VACUUM);


// System or main IOB Temperature
ZBUS_CHAN_DECLARE(ZBUS_CHAN_SYSTEM_TEMPERATURE);


ZBUS_OBS_DECLARE(alert_temp_listener);
ZBUS_OBS_DECLARE(telemetry_temp_sub);

// Accessory IOB Temperature
ZBUS_CHAN_DECLARE(ZBUS_CHAN_ACCESSORY_TEMPERATURE);
ZBUS_OBS_DECLARE(telemetry_accy_temp_sub);

//  Control Status
ZBUS_CHAN_DECLARE(ZBUS_CHAN_CONTROL_STATUS);

// control status aaccessory
ZBUS_CHAN_DECLARE(ZBUS_CHAN_ACCESSORY_CONTROL_STATUS);

#endif /* ZBUS_CHANNELS_H */
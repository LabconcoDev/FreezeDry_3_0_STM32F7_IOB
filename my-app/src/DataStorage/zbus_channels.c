#include "zbus_channels.h"

ZBUS_CHAN_DEFINE(ZBUS_CHAN_SYSTEM_TEMPERATURE,
    struct sys_temp_msg,
    NULL,
    NULL,
    ZBUS_OBSERVERS(alert_temp_listener, telemetry_temp_sub),
    ZBUS_MSG_INIT(
        .collector_dC         = 0x7FFF,
        .ambient_dC           = 0x7FFF,
        .hx_1st_stage_dC      = 0x7FFF,
        .hx_2nd_stage_dC      = 0x7FFF,
        .suction_1st_stage_dC = 0x7FFF,
        .suction_2nd_stage_dC = 0x7FFF,
        .shell_or_mini_dC     = 0x7FFF,
        .triad_lexsol_dC      = 0x7FFF,
        .triad_shelf_dC       = 0x7FFF,
        .triad_sample_dC      = 0x7FFF,
        .timestamp_ms         = 0,
        .valid                = false
    )
);



ZBUS_CHAN_DEFINE(ZBUS_CHAN_ACCESSORY_TEMPERATURE,
    struct accy_temp_msg,
    NULL,
    NULL,
    ZBUS_OBSERVERS(telemetry_accy_temp_sub),
    ZBUS_MSG_INIT(
        .st_lexsol_dC = 0x7FFF,
        .shelf_dC     = { 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF },
        .sample_dC    = { 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF },   
        .timestamp_ms = 0,
        .valid        = false
    )
);

//  Control Status
ZBUS_CHAN_DEFINE(ZBUS_CHAN_CONTROL_STATUS,
    struct sys_control_status_msg,
    NULL,
    NULL,
    ZBUS_OBSERVERS_EMPTY,
    ZBUS_MSG_INIT(
        .mode                   = CTRL_MODE_OFF,
        .program_segment_no     = 0,
        .program_time_left_mins = 0,
        .program_ramp_hold      = 0,
        .output_flags           = 0,
        .vacuum_protect_enabled = false,
        .vacuum_setpoint_mbar   = 0,
        .mini_chamber_target_dC = 0x7FFF,
        .shelf_heat_flags       = 0,
        .shelf_target_dC        = 0x7FFF,
        .timestamp_ms           = 0,
        .valid                  = false
    )
);


/* Accessory Control Status */
ZBUS_CHAN_DEFINE(ZBUS_CHAN_ACCESSORY_CONTROL_STATUS,
    struct accy_control_status_msg,
    NULL,
    NULL,
    ZBUS_OBSERVERS_EMPTY,
    ZBUS_MSG_INIT(
        .shelf_heater_on        = false,
        .program_running        = false,
        .program_segment_no     = 0,
        .program_time_left_mins = 0,
        .program_ramp_hold      = 0,
        .shelf_target_dC        = 0x7FFF,
        .program_control_flags  = 0,
        .program_vacuum_mbar    = 0,
        .timestamp_ms           = 0,
        .valid                  = false
    )
);

/* Main IOB Vacuum Sensors */
ZBUS_CHAN_DEFINE(ZBUS_CHAN_SYSTEM_VACUUM,
    struct sys_vac_msg,
    NULL,
    NULL,
    ZBUS_OBSERVERS_EMPTY,
    ZBUS_MSG_INIT(
        .system_vac_mbar      = 0xFFFF,
        .sample_vac_mbar      = { 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF },
        .epd_addon_vac_1_mbar = 0xFFFF,
        .epd_addon_vac_2_mbar = 0xFFFF,
        .timestamp_ms         = 0,
        .valid                = false
    )
);
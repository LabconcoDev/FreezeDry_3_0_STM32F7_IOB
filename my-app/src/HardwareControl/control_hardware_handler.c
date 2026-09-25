/*This file will write to hardware ports (ON/OFF). This file is dedicated for FD/ Triad solely */
/*
 * control_hardware_handler.c
 *
 * Implements the physical hardware toggling for the local base unit.
 */

#include "control_hardware_handler.h"
#include "protocol_defs.h"  // For your CONTROL_REFRIG_RLY, etc.
#include <zephyr/logging/log.h>

// Include your local platform GPIO wrappers here
// #include "platform_gpio.h" 

LOG_MODULE_REGISTER(control_hardware, LOG_LEVEL_INF);

/* 
 * Helper functions to toggle specific pins. 
 */
static void set_refrig_relay(uint8_t state) 
{
    if (state > 0) 
    {
        LOG_INF("Hardware: Refrigeration Relay ON");
        // platform_gpio_set(REFRIG_PIN, 1);
    } 
    else 
    {
        LOG_INF("Hardware: Refrigeration Relay OFF");
        // platform_gpio_set(REFRIG_PIN, 0);
    }
}

static void set_vacuum_relay(uint8_t state) 
{
    if (state > 0) 
    {
        LOG_INF("Hardware: Vacuum Relay ON");
        // platform_gpio_set(VACUUM_PIN, 1);
    } 
    else 
    {
        LOG_INF("Hardware: Vacuum Relay OFF");
        // platform_gpio_set(VACUUM_PIN, 0);
    }
}



void fd_control_hardware_init(void) {
    LOG_INF("Initializing Base Unit Control Hardware GPIOs...");
    // Initialize GPIO pins here (e.g., configure as outputs, set default states)
}



void ControlHardware_Handler(uint8_t action, const uint8_t *data) 
{

    uint8_t target_state = data[0]; // Assuming byte 0 is On(1) / Off(0)

    switch (action) {
        
        case CONTROL_REFRIG_RLY:
            set_refrig_relay(target_state);
            break;

        case CONTROL_VACUUM_RLY:
            set_vacuum_relay(target_state);
            break;

        case CONTROL_PURGE_RLY:
            LOG_INF("Hardware: Purge Relay set to %d", target_state);
            // set_purge_relay(target_state);
            break;

        case CONTROL_SHELL_RLY:
            LOG_INF("Hardware: Shell Relay set to %d", target_state);
            // set_shell_relay(target_state);
            break;

        case CONTROL_REFRIG_DLY_RLY:
            LOG_INF("Hardware: Refrigeration Delay Relay set to %d", target_state);
            // set_refrig_dly_relay(target_state);
            break;

        case CONTROL_VENT_VALVE:
            LOG_INF("Hardware: Vent Valve set to %d", target_state);
            // set_refrig_dly_relay(target_state);
            break;

        case CONTROL_DEFROST_RELAY :
            LOG_INF("Hardware: Defrost Relay set to %d", target_state);
            // set_refrig_dly_relay(target_state);
            break;
        case CONTROL_MINI_CHAMBER_HTR:
            LOG_INF("Hardware: Mini Chamber heater set to %d", target_state);
            // set_refrig_dly_relay(target_state);
            break;
        case CONTROL_LEXSOL_PUMP_RLY :
            LOG_INF("Hardware: Lexsol Pump Relay set to %d", target_state);
            // set_refrig_dly_relay(target_state);
            break;
        case CONTROL_LEXSOL_HTR :
            LOG_INF("Hardware: Lexsol Heater set to %d", target_state);
            // set_refrig_dly_relay(target_state);
            break;

        case CONTROL_REF_VALVE_RLY  :
            LOG_INF("Hardware: Ref valve set to %d", target_state);
            // set_refrig_dly_relay(target_state);
            break;
        case CONTROL_TILT_VALVE  :
            LOG_INF("Hardware: Tilt Valve set to %d", target_state);
            // set_refrig_dly_relay(target_state);
            break;
        case CONTROL_5V_FAN_DRV  :
            LOG_INF("Hardware: 5V Fan driver set to %d", target_state);
            // set_refrig_dly_relay(target_state);
            break;


        // Note: Do NOT put Accessory actions in here!
        // This file is strictly for the Main board's physical pins.

        default:
            LOG_ERR("Hardware Error: Attempted to execute unsupported action %d", action);
            break;
    }
}
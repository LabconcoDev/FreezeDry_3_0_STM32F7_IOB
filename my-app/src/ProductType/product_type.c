// /*
// This files contains logic to determine the product and accessory type
// Step 1. Read product type from EEPROM.
// Step 2. If it is unprogrammed board, load Product ID = 0x3F(as per FD 2.0)
// Step 3. If programmed board, collect product type from EEPROM.
// Step 4. If model number = 123456789, board is in CM Test Phase. Utilize 
//         GPIO pin state to determine the Product Type.
// Step 5. This main firmware could be either FD/Triad. If Tilt Valve present 
//         implies it is a Triad. else FD.
// Step 5. Accessory information can be retrieved from the Message from 2dary IOB.
// */

#include "build_config.h"
#include "product_type.h"
#include <zephyr/logging/log.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

LOG_MODULE_REGISTER(product_type, LOG_LEVEL_INF);

extern void ReadAccessoryType(void);

/* =========================================================
 * LOW-LEVEL HARDWARE SENSE FUNCTIONS
 * ========================================================= */
bool Tilt_Valve_Present(void)
{
#ifdef SIMULATION
    /* In simulation: true if simulating TRIAD, false if simulating FD */
    return (SIMULATE_AS_TRIAD == 1);
#else
    /* TODO: Read physical STM32 GPIO pin for Tilt Valve sense circuit */
    // return (gpio_pin_get_dt(&tilt_sense_gpio) > 0);
    return true;
#endif
}

bool Vent_Valve_Present(void)
{
#ifdef SIMULATION
    /* Both TRIAD and FD have a Vacuum Vent Valve */
    return true;
#else
    /* TODO: Read physical STM32 GPIO pin for Vent Valve sense circuit */
    // return (gpio_pin_get_dt(&vent_sense_gpio) > 0);
    return true;
#endif
}

/* =========================================================
 * PRODUCT Type Discovery
 * ========================================================= */
uint8_t FindProductType(void)
{
    bool tilt_present = Tilt_Valve_Present();
    bool vent_present = Vent_Valve_Present();

    /* TRIAD has Tilt Valve + Vent Valve */
    if (tilt_present && vent_present)
    {
        LOG_INF("Product Identified: IAM_TRIAD (0x%02X)", IAM_TRIAD);
        return IAM_TRIAD;
    }
    /* FD has Vent Valve, but NO Tilt Valve */
    else if (!tilt_present && vent_present)
    {
        LOG_INF("Product Identified: IAM_FD (0x%02X)", IAM_FD);
        return IAM_FD;
    }
    else
    {
        LOG_WRN("Product Unknown (tilt=%d, vent=%d) -> IAM_UNKNOWN (0x%02X)",
                tilt_present, vent_present, IAM_UNKNOWN);
        return IAM_UNKNOWN;
    }
}

uint8_t FindAccessoryType(void)
{
#ifdef SIMULATION
    /* In simulation without a 2nd board connected on UART4 */
    LOG_INF("[SIM] No external Accessory IOB attached -> IAM_UNKNOWN (0x%02X)", IAM_UNKNOWN);
    return IAM_UNKNOWN;
#else
    /* Real Hardware: Send request on UART4 to Secondary IOB (BT or ST) */
    ReadAccessoryType();
    return IAM_UNKNOWN; /* Updated asynchronously when Secondary IOB replies */
#endif
}
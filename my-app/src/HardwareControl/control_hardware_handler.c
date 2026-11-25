#include "control_hardware_handler.h"
#include "protocol_defs.h"
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>

LOG_MODULE_REGISTER(control_hardware, LOG_LEVEL_INF);

static const struct gpio_dt_spec refrig_gpio = GPIO_DT_SPEC_GET(DT_ALIAS(collector_gpio), gpios); /* PB1 */
static const struct gpio_dt_spec vacuum_gpio = GPIO_DT_SPEC_GET(DT_ALIAS(vacuum_gpio), gpios);    /* PD13 */

void fd_control_hardware_init(void)
{
    if (gpio_is_ready_dt(&refrig_gpio))
    {
        gpio_pin_configure_dt(&refrig_gpio, GPIO_OUTPUT_INACTIVE);
    }
    if (gpio_is_ready_dt(&vacuum_gpio))
    {
        gpio_pin_configure_dt(&vacuum_gpio, GPIO_OUTPUT_INACTIVE);
    }
    LOG_INF("Control Hardware GPIOs initialized (Refrig=PB1, Vacuum=PD13)");
}

static int set_gpio_output(const struct gpio_dt_spec *spec, uint8_t state, const char *name)
{
    int val = (state > 0U) ? 1 : 0;

    if (!gpio_is_ready_dt(spec))
    {
        LOG_ERR("Hardware: %s GPIO not ready", name);
        return -ENODEV;
    }

    int ret = gpio_pin_set_dt(spec, val);
    if (ret == 0)
    {
        LOG_INF("Hardware: %s %s", name, val ? "ON" : "OFF");
    }
    return ret;
}

int ControlHardware_Handler(uint8_t action, const uint8_t *data)
{
    uint8_t target_state = data[0];

    switch (action)
    {
        case CONTROL_REFRIG_RLY:
            return set_gpio_output(&refrig_gpio, target_state, "Refrig Relay");

        case CONTROL_VACUUM_RLY:
            return set_gpio_output(&vacuum_gpio, (target_state & CTRL_VAC_ON_OFF_MASK), "Vacuum Relay");

        case CONTROL_PURGE_RLY:
        case CONTROL_SHELL_FREEZER_RLY:
        case CONTROL_VACUUM_VENT_VALVE:
        case CONTROL_DEFROST_RLY:
        case CONTROL_MINI_CHAMBER_HEATER:
        case CONTROL_MANUAL_SHELF_HEAT:
            LOG_INF("Hardware: Action 0x%02X -> %u (pin not mapped yet)", action, target_state);
            return 0; /* Return 0 for now until pins are assigned in .dts */

        default:
            LOG_ERR("Hardware Error: Unsupported action 0x%02X", action);
            return -EINVAL;
    }
}


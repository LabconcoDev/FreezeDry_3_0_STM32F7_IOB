#include <zephyr/kernel.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/logging/log.h>
#include "thread_telemetry.h"
#include "product_type.h"
#include "main_iob_data.h"
#include "zbus_channels.h"
#include "sensor_handler.h"
#include "protocol_defs.h"

LOG_MODULE_REGISTER(task_telemetry, LOG_LEVEL_INF);

/* Queue up to 8 channel notifications */
ZBUS_SUBSCRIBER_DEFINE(telemetry_temp_sub, 8);
ZBUS_SUBSCRIBER_DEFINE(telemetry_accy_temp_sub, 8);

extern uint8_t u8_ProductID;

int telemetry_task_init(void)
{
    return 0;
}

static bool push_if_changed(int16_t curr, int16_t prev, bool prev_valid, uint8_t action)
{
    if (curr != 0x7FFF && (!prev_valid || curr != prev))
    {
        PushSensorData(action);
        return true;
    }
    return false;
}

void telemetry_task_run(void)
{
    const struct zbus_channel *chan;
    static struct sys_temp_msg last_sent = { .valid = false };

    if (zbus_sub_wait(&telemetry_temp_sub, &chan, K_FOREVER) == 0)
    {
        if (chan == &ZBUS_CHAN_SYSTEM_TEMPERATURE)
        {
            struct sys_temp_msg msg;

            if (zbus_chan_read(&ZBUS_CHAN_SYSTEM_TEMPERATURE, &msg, K_MSEC(5)) == 0 && msg.valid)
            {
                bool changed = false;

                changed |= push_if_changed(msg.collector_dC,         last_sent.collector_dC,         last_sent.valid, SENSOR_COLLECTOR_TEMP);
                changed |= push_if_changed(msg.ambient_dC,           last_sent.ambient_dC,           last_sent.valid, SENSOR_AMBIENT_TEMPERATURE);
                changed |= push_if_changed(msg.hx_1st_stage_dC,      last_sent.hx_1st_stage_dC,      last_sent.valid, SENSOR_FIRST_STAGE_HEAT_EXCHANGER_OUTLET_TEMP);
                changed |= push_if_changed(msg.hx_2nd_stage_dC,      last_sent.hx_2nd_stage_dC,      last_sent.valid, SENSOR_SECOND_STAGE_HEAT_EXCHANGER_OUTLET_TEMP);
                changed |= push_if_changed(msg.suction_1st_stage_dC, last_sent.suction_1st_stage_dC, last_sent.valid, SENSOR_FIRST_STAGE_SUCTION_TEMP);
                changed |= push_if_changed(msg.suction_2nd_stage_dC, last_sent.suction_2nd_stage_dC, last_sent.valid, SENSOR_SECOND_STAGE_SUCTION_TEMP);

                if (u8_ProductID == IAM_FD)
                {
                    uint8_t opt_hw = MainIOBShadowMemory.FD.Digital_Sensors.HardwareDetect.optional_hw;
                    uint8_t aux_act = (opt_hw == HW_OPT_MINI_CHAMBER)
                                      ? SENSOR_MINI_CHAMBER_TEMP
                                      : SENSOR_SHELL_FREEZER_TEMP_SENSOR;
                    changed |= push_if_changed(msg.shell_or_mini_dC, last_sent.shell_or_mini_dC, last_sent.valid, aux_act);
                }
                else if (u8_ProductID == IAM_TRIAD)
                {
                    changed |= push_if_changed(msg.triad_lexsol_dC, last_sent.triad_lexsol_dC, last_sent.valid, SENSOR_LEXSOL_TEMP);
                    changed |= push_if_changed(msg.triad_shelf_dC,  last_sent.triad_shelf_dC,  last_sent.valid, SENSOR_SHELF1_TEMP);
                    changed |= push_if_changed(msg.triad_sample_dC, last_sent.triad_sample_dC, last_sent.valid, SENSOR_SHELF1_SAMPLE_TEMP);
                }

                if (changed)
                {
                    LOG_INF("[ZBUS TELEMETRY] T_coll=%d dC | T_amb=%d dC | T_lex=%d dC | ts=%u ms",
                            msg.collector_dC,
                            msg.ambient_dC,
                            msg.triad_lexsol_dC,
                            msg.timestamp_ms);
                    last_sent = msg;
                }
            }
        }
    }
}
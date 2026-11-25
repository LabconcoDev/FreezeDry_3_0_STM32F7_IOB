/*
 * Copyright (c) 2026 Labconco Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>
#include "stack_debug.h"
#include "DataStorage/zbus_channels.h"
#include "communication/protocol_defs.h"
LOG_MODULE_REGISTER(thread_alerts, LOG_LEVEL_INF);

/* Thresholds in 0.1 C (dC) */
#define COLLECTOR_MAX_TEMP_DC   350   /*  35.0 C */
#define COLLECTOR_MIN_TEMP_DC  -450   /* -45.0 C */

struct alert_event_msg {
    uint8_t alert_action;
    int16_t value_dC;
    bool    in_fault;
};

K_MSGQ_DEFINE(alert_evt_msgq, sizeof(struct alert_event_msg), 8, 4);

/* Synchronous Zbus listener: runs in publisher's thread (4_Sensor_Thread) */
static void alert_temp_listener_cb(const struct zbus_channel *chan)
{
    const struct sys_temp_msg *msg = zbus_chan_const_msg(chan);
    static bool was_in_fault = false;

    if (!msg->valid) {
        return;
    }

    static uint32_t dbg_cnt = 0;
    if ((++dbg_cnt % 20U) == 0U) {
        printk("[ZBUS LISTENER] thread=%p (%s) | chan=%s | T_coll=%d\n",
               k_current_get(),
               k_thread_name_get(k_current_get()),
               zbus_chan_name(chan),
               msg->collector_dC);
    }

    bool out_of_bounds = (msg->collector_dC > COLLECTOR_MAX_TEMP_DC) ||
                         (msg->collector_dC < COLLECTOR_MIN_TEMP_DC);

    /* Forward only on fault entry/exit */
    if (out_of_bounds != was_in_fault) {
        was_in_fault = out_of_bounds;
        struct alert_event_msg evt =
        {
            .alert_action = ALERT_COLLECTOR_TEMP_VAR,
            .value_dC     = msg->collector_dC,
            .in_fault     = out_of_bounds,
        };
        k_msgq_put(&alert_evt_msgq, &evt, K_NO_WAIT);
    }
}

ZBUS_LISTENER_DEFINE(alert_temp_listener, alert_temp_listener_cb);

int thread_alerts_init(void)
{
    print_stack_usage();
    return 0;
}

void thread_alerts_run(void)
{
    struct alert_event_msg evt;

    /* Block until an alert transition arrives (prevents starving lower-priority threads) */
    if (k_msgq_get(&alert_evt_msgq, &evt, K_FOREVER) == 0) {
        printk("[ALERT THREAD] thread=%p (%s) | t=%u ms | action=0x%02X\n",
               k_current_get(),
               k_thread_name_get(k_current_get()),
               k_uptime_get_32(),
               evt.alert_action);
        if (evt.in_fault) {
            LOG_WRN("[ZBUS ALERT] Action=0x%02X TRIGGERED | Collector=%d dC",
                    evt.alert_action, evt.value_dC);
            /* TODO: Send TYPE_ALERT packet to Display TX */
        } else {
            LOG_INF("[ZBUS ALERT] Action=0x%02X CLEARED | Collector=%d dC",
                    evt.alert_action, evt.value_dC);
        }
    
    }
}

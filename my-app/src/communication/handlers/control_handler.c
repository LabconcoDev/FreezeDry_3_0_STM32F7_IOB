/*
 * control_handler.c
 * Handles TYPE_CONTROL (0x02) packets per Freeze Dry 3.0 ICD
 */
#include "parser.h"
#include "control_accessory_handler.h"
#include "thread_control.h"
#include "zbus_channels.h"
#include "control_handler.h"
#include <zephyr/logging/log.h>
#include "serializer.h"
#include "product_type.h"
#include "main_iob_data.h"
#include <zephyr/kernel.h>
#include <string.h>

LOG_MODULE_REGISTER(control_handler, LOG_LEVEL_INF);

extern void fd_protocol_send_dma(const uint8_t *data, size_t len);
extern void fd_protocol_send_accessory_iob_dma(const uint8_t *data, size_t len);

extern struct k_mutex shadow_memory_mutex;
extern uint8_t u8_ProductID;

static void pushControlState(uint8_t action, uint8_t current_state);
void pushProgramStatus(void);

static void forward_to_app(uint8_t cls, uint8_t action, const uint8_t *data, size_t data_len)
{
    uint8_t tx_buf[32];
    size_t frame_len = fd_serializer_build_frame(tx_buf, sizeof(tx_buf), cls, TYPE_CONTROL, action, data, data_len);
    if (frame_len > 0)
    {
        fd_protocol_send_dma(tx_buf, frame_len);
    }
    else
    {
        LOG_ERR("Failed to serialize forward frame (action 0x%02X)", action);
    }
}



static uint8_t read_control_bit(uint16_t bit)
{
    struct sys_control_status_msg ctrl;
    if (zbus_chan_read(&ZBUS_CHAN_CONTROL_STATUS, &ctrl, K_NO_WAIT) == 0 && ctrl.valid)
    {
        return ((ctrl.output_flags & bit) != 0U) ? 0x01U : 0x00U;
    }
    return 0x00U;
}


static void update_accy_control_zbus(uint8_t action, const uint8_t *data, size_t data_len)
{
    static struct accy_control_status_msg s_accy_ctrl =
    {
        .shelf_target_dC = 0x7FFF,
    };

    switch (action)
    {
        case CONTROL_MANUAL_SHELF_HEAT:
            if (data_len >= sizeof(fd_ctrl_manual_shelf_payload_t))
            {
                fd_ctrl_manual_shelf_payload_t shelf;
                memcpy(&shelf, data, sizeof(shelf));
                s_accy_ctrl.shelf_heater_on = (shelf.flags & CTRL_SHELF_ON_OFF_MASK) != 0;
                s_accy_ctrl.shelf_target_dC = shelf.target_temp_dC;
            }
            else if (data_len >= 1)
            {
                s_accy_ctrl.shelf_heater_on = (data[0] & CTRL_SHELF_ON_OFF_MASK) != 0;
            }
            break;

        case CONTROL_PROGRAM_SEGMENTS:
            if (data_len >= sizeof(fd_program_segment_payload_t))
            {
                fd_program_segment_payload_t seg;
                memcpy(&seg, data, sizeof(seg));
                s_accy_ctrl.program_segment_no     = seg.segment_no;
                s_accy_ctrl.program_time_left_mins = seg.hold_time_mins;
                s_accy_ctrl.program_control_flags  = seg.control_flags;
                s_accy_ctrl.program_vacuum_mbar    = seg.vacuum_mbar;
            }
            break;

        case CONTROL_PROGRAM_RUN:
            if (data_len >= 1)
            {
                s_accy_ctrl.program_running = (data[0] != 0);
            }
            break;

        case CONTROL_PROGRAM_STATUS:
            if (data_len >= sizeof(fd_program_status_payload_t))
            {
                fd_program_status_payload_t st;
                memcpy(&st, data, sizeof(st));
                s_accy_ctrl.program_segment_no     = st.segment_no;
                s_accy_ctrl.program_time_left_mins = st.time_left_mins;
                s_accy_ctrl.program_ramp_hold      = st.ramp_hold;
            }
            break;

        default:
            return;
    }

    s_accy_ctrl.timestamp_ms = k_uptime_get_32();
    s_accy_ctrl.valid        = true;
    zbus_chan_pub(&ZBUS_CHAN_ACCESSORY_CONTROL_STATUS, &s_accy_ctrl, K_NO_WAIT);
}



void fd_control_handler_process(fd_interface_t source_if, uint8_t cls, uint8_t action, const uint8_t *data, size_t data_len)
{
    printk("[CTRL HANDLER] src=%d, action=0x%02X, cls=0x%02X, len=%zu\n", source_if, action, cls, data_len);

    /* Forward Accessory PUSH messages (e.g. CONTROL_PROGRAM_STATUS) up to APP */
    if (source_if == INTERFACE_ACCESSORY_IOB && cls == CLASS_PUSH)
    {
        update_accy_control_zbus(action, data, data_len);
        forward_to_app(CLASS_PUSH, action, data, data_len);
        return;
    }

        switch (action)
    {
        /* -------------------------------------------------------- */
        /* BASE UNIT RELAYS & VALVES (FD, TRIAD)                    */
        /* -------------------------------------------------------- */
        case CONTROL_REFRIG_RLY:
            if (cls == CLASS_READ)
            {
                pushControlState(action, read_control_bit(CTRL_OUT_REFRIG_1ST));
            }
            else if (cls == CLASS_WRITE && data_len >= 1)
            {
                control_send_cmd(action, data, data_len);
            }
            break;

        case CONTROL_PURGE_RLY:
            if (cls == CLASS_READ)
            {
                uint16_t bit = (u8_ProductID == IAM_TRIAD) ? CTRL_OUT_TRIAD_PREFREEZE : CTRL_OUT_FD_PURGE_VALVE;
                pushControlState(action, read_control_bit(bit));
            }
            else if (cls == CLASS_WRITE && data_len >= 1)
            {
                control_send_cmd(action, data, data_len);
            }
            break;

        case CONTROL_SHELL_FREEZER_RLY:
            if (cls == CLASS_READ)
            {
                uint16_t bit = (u8_ProductID == IAM_TRIAD) ? CTRL_OUT_TRIAD_LEXSOL_PUMP : CTRL_OUT_FD_SHELL_FREEZER;
                pushControlState(action, read_control_bit(bit));
            }
            else if (cls == CLASS_WRITE && data_len >= 1)
            {
                control_send_cmd(action, data, data_len);
            }
            break;

        case CONTROL_DEFROST_RLY:
            if (cls == CLASS_READ)
            {
                pushControlState(action, read_control_bit(CTRL_OUT_DEFROST));
            }
            else if (cls == CLASS_WRITE && data_len >= 1)
            {
                control_send_cmd(action, data, data_len);
            }
            break;

        case CONTROL_VACUUM_VENT_VALVE:
            if (cls == CLASS_READ)
            {
                pushControlState(action, read_control_bit(CTRL_OUT_VAC_VENT_VALVE));
            }
            else if (cls == CLASS_WRITE && data_len >= 1)
            {
                control_send_cmd(action, data, data_len);
            }
            break;

        case CONTROL_VACUUM_RLY:
            if (cls == CLASS_READ)
            {
                pushControlState(CONTROL_VACUUM_RLY, read_control_bit(CTRL_OUT_VACUUM_PUMP));
            }
            else if (cls == CLASS_WRITE && data_len >= sizeof(fd_ctrl_vacuum_payload_t))
            {
                fd_ctrl_vacuum_payload_t vac;
                memcpy(&vac, data, sizeof(vac));
                LOG_INF("Vacuum RLY: flags=0x%02X, setpoint=%u (mbar*1000)", vac.flags, vac.vacuum_setpoint);
                control_send_cmd(action, data, data_len);
            }
            break;

        case CONTROL_MINI_CHAMBER_HEATER:
            if (cls == CLASS_READ)
            {
                pushControlState(CONTROL_MINI_CHAMBER_HEATER, read_control_bit(CTRL_OUT_FD_MINI_HEATER));
            }
            else if (cls == CLASS_WRITE && data_len >= sizeof(fd_ctrl_mini_chamber_payload_t))
            {
                fd_ctrl_mini_chamber_payload_t mc;
                memcpy(&mc, data, sizeof(mc));
                LOG_INF("Mini Chamber: on=%u, target=%d (C*10)", mc.on_off, mc.target_temp_dC);
                control_send_cmd(action, data, data_len);
            }
            break;

        case CONTROL_AUTO_MODE:
            if (cls == CLASS_WRITE && data_len >= 1)
            {
                LOG_INF("Auto Mode: %s", data[0] ? "ON" : "OFF");
                control_send_cmd(action, data, data_len);
            }
            break;

        /* -------------------------------------------------------- */
        /* TRIAD & ACCESSORY SHELF / PROGRAM CONTROL                */
        /* -------------------------------------------------------- */
        case CONTROL_MANUAL_SHELF_HEAT:
            if (cls == CLASS_WRITE && data_len >= sizeof(fd_ctrl_manual_shelf_payload_t))
            {
                fd_ctrl_manual_shelf_payload_t shelf;
                memcpy(&shelf, data, sizeof(shelf));
                LOG_INF("Manual Shelf Heat: flags=0x%02X, target=%d (C*10)", shelf.flags, shelf.target_temp_dC);

                if (u8_ProductID == IAM_TRIAD)
                {
                    control_send_cmd(action, data, data_len);
                }
                else
                {
                    accessory_control_handler_process(action, data, data_len);
                }
            }
            break;

        case CONTROL_PROGRAM_SEGMENTS:
            if (cls == CLASS_WRITE && data_len >= sizeof(fd_program_segment_payload_t))
            {
                fd_program_segment_payload_t seg;
                memcpy(&seg, data, sizeof(seg));

                k_mutex_lock(&shadow_memory_mutex, K_FOREVER);
                if (u8_ProductID == IAM_TRIAD)
                {
                    MainIOBShadowMemory.TRIAD.Program_State.segment_no              = seg.segment_no;
                    MainIOBShadowMemory.TRIAD.Program_State.control_flags           = seg.control_flags;
                    MainIOBShadowMemory.TRIAD.Program_State.shelf_target_dC         = seg.shelf_temp_dC;
                    MainIOBShadowMemory.TRIAD.Program_State.ramp_rate               = seg.ramp_rate;
                    MainIOBShadowMemory.TRIAD.Program_State.vac_target_mbar         = seg.vacuum_mbar;
                    MainIOBShadowMemory.TRIAD.Program_State.hold_time_total_min     = seg.hold_time_mins;
                    MainIOBShadowMemory.TRIAD.Program_State.hold_time_remaining_min = seg.hold_time_mins;
                }
                k_mutex_unlock(&shadow_memory_mutex);

                if (u8_ProductID != IAM_TRIAD)
                {
                    accessory_control_handler_process(action, data, data_len);
                }

                LOG_INF("Program Segment %u: flags=0x%02X, temp=%d, ramp=%u, vac=%u, time=%u min",
                        seg.segment_no, seg.control_flags, seg.shelf_temp_dC,
                        seg.ramp_rate, seg.vacuum_mbar, seg.hold_time_mins);
            }
            break;

        case CONTROL_PROGRAM_RUN:
            if (cls == CLASS_WRITE && data_len >= 1)
            {
                bool start = (data[0] != 0);
                LOG_INF("Program RUN: %s", start ? "START" : "STOP");

                k_mutex_lock(&shadow_memory_mutex, K_FOREVER);
                if (u8_ProductID == IAM_TRIAD)
                {
                    MainIOBShadowMemory.TRIAD.Program_State.is_running = start;
                }
                k_mutex_unlock(&shadow_memory_mutex);

                if (u8_ProductID != IAM_TRIAD)
                {
                    accessory_control_handler_process(action, data, data_len);
                }
            }
            break;

        case CONTROL_PROGRAM_STATUS:
            if (cls == CLASS_READ)
            {
                pushProgramStatus();
            }
            break;

        default:
            LOG_WRN("Unknown Control Action: 0x%02X", action);
            break;
    }
}

static void pushControlState(uint8_t action, uint8_t current_state)
{
    uint8_t tx_buf[32];
    size_t frame_len = fd_serializer_build_frame(tx_buf, sizeof(tx_buf),
                                                 CLASS_PUSH,
                                                 TYPE_CONTROL,
                                                 action,
                                                 &current_state,
                                                 1);
    if (frame_len > 0)
    {
        fd_protocol_send_dma(tx_buf, frame_len);
        LOG_INF("Pushed Control 0x%02X state: %u", action, current_state);
    }
    else
    {
        LOG_ERR("Failed to serialize PUSH packet for action 0x%02X", action);
    }
}

void pushProgramStatus(void)
{
    fd_program_status_payload_t status = { 0 };

    k_mutex_lock(&shadow_memory_mutex, K_FOREVER);
    status.segment_no     = MainIOBShadowMemory.TRIAD.Program_State.segment_no;
    status.time_left_mins = MainIOBShadowMemory.TRIAD.Program_State.hold_time_remaining_min;
    status.ramp_hold      = 0; /* TODO: 0 = Ramp, 1 = Hold */
    k_mutex_unlock(&shadow_memory_mutex);

    uint8_t tx_buf[32];
    size_t frame_len = fd_serializer_build_frame(tx_buf, sizeof(tx_buf),
                                                 CLASS_PUSH,
                                                 TYPE_CONTROL,
                                                 CONTROL_PROGRAM_STATUS,
                                                 (const uint8_t *)&status,
                                                 sizeof(status));
    if (frame_len > 0)
    {
        fd_protocol_send_dma(tx_buf, frame_len);
    }
}

/* ========================================================
 * PROGRAM NOTIFICATION (To send alerts to Display)
 * ======================================================== */
void pushProgramSegmentComplete(uint8_t completed_seg_no)
{
    uint8_t tx_buf[32];
    size_t frame_len = fd_serializer_build_frame(tx_buf, sizeof(tx_buf),
                                                 CLASS_PUSH,
                                                 TYPE_ALERT,
                                                 ALERT_PROGRAM_SEGMENT_COMPLETE,
                                                 &completed_seg_no,
                                                 1);
    if (frame_len > 0)
    {
        fd_protocol_send_dma(tx_buf, frame_len);
        LOG_INF("Pushed ALERT_PROGRAM_SEGMENT_COMPLETE: Seg=%u", completed_seg_no);
    }
}

void pushProgramComplete(void)
{
    uint8_t tx_buf[32];
    size_t frame_len = fd_serializer_build_frame(tx_buf, sizeof(tx_buf),
                                                 CLASS_PUSH,
                                                 TYPE_ALERT,
                                                 ALERT_PROGRAM_COMPLETE,
                                                 NULL,
                                                 0);
    if (frame_len > 0)
    {
        fd_protocol_send_dma(tx_buf, frame_len);
        LOG_INF("Pushed ALERT_PROGRAM_COMPLETE");
    }
}
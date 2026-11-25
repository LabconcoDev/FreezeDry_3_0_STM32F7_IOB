#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/smf.h>
#include <string.h>
#include <stddef.h>
#include "thread_control.h"
#include "boot_sequence.h"
#include "product_type.h"
#include "zbus_channels.h"
#include "communication/protocol_defs.h"
#include "HardwareControl/control_hardware_handler.h"

LOG_MODULE_REGISTER(thread_control, LOG_LEVEL_INF);

extern uint8_t u8_ProductID;

K_MSGQ_DEFINE(ctrl_cmd_msgq, sizeof(struct ctrl_cmd_msg), 16, 4);

static struct sys_control_status_msg s_ctrl_status =
{
    .mode                   = CTRL_MODE_OFF,
    .mini_chamber_target_dC = 0x7FFF,
    .shelf_target_dC        = 0x7FFF,
    .valid                  = true,
};

int control_send_cmd(uint8_t action, const uint8_t *data, size_t data_len)
{
    struct ctrl_cmd_msg cmd = { 0 };
    cmd.action   = action;
    cmd.data_len = (data_len > sizeof(cmd.data)) ? sizeof(cmd.data) : (uint8_t)data_len;

    if (data != NULL && cmd.data_len > 0U)
    {
        memcpy(cmd.data, data, cmd.data_len);
    }

    return k_msgq_put(&ctrl_cmd_msgq, &cmd, K_NO_WAIT);
}

static void set_output_bit(uint16_t bit, bool on)
{
    if (on)
    {
        s_ctrl_status.output_flags |= bit;
    }
    else
    {
        s_ctrl_status.output_flags &= (uint16_t)(~bit);
    }
}

static void apply_ui_command(const struct ctrl_cmd_msg *cmd)
{
    switch (cmd->action)
    {
        case CONTROL_REFRIG_RLY:
            if (ControlHardware_Handler(cmd->action, cmd->data) == 0)
            {
                s_ctrl_status.mode = CTRL_MODE_MANUAL;
                set_output_bit(CTRL_OUT_REFRIG_1ST, cmd->data[0] != 0);
            }
            break;

        case CONTROL_VACUUM_RLY:
            if (cmd->data_len >= sizeof(fd_ctrl_vacuum_payload_t))
            {
                fd_ctrl_vacuum_payload_t vac;
                memcpy(&vac, cmd->data, sizeof(vac));
                if (ControlHardware_Handler(cmd->action, cmd->data) == 0)
                {
                    s_ctrl_status.mode = CTRL_MODE_MANUAL;
                    set_output_bit(CTRL_OUT_VACUUM_PUMP, (vac.flags & CTRL_VAC_ON_OFF_MASK) != 0);
                    s_ctrl_status.vacuum_protect_enabled = (vac.flags & CTRL_VAC_PROTECT_FLAG) != 0;
                    s_ctrl_status.vacuum_setpoint_mbar   = vac.vacuum_setpoint;
                }
            }
            break;

        case CONTROL_PURGE_RLY:
            if (ControlHardware_Handler(cmd->action, cmd->data) == 0)
            {
                s_ctrl_status.mode = CTRL_MODE_MANUAL;
                if (u8_ProductID == IAM_TRIAD)
                {
                    set_output_bit(CTRL_OUT_TRIAD_PREFREEZE, cmd->data[0] != 0);
                }
                else
                {
                    set_output_bit(CTRL_OUT_FD_PURGE_VALVE, cmd->data[0] != 0);
                }
            }
            break;

        case CONTROL_SHELL_FREEZER_RLY:
            if (ControlHardware_Handler(cmd->action, cmd->data) == 0)
            {
                s_ctrl_status.mode = CTRL_MODE_MANUAL;
                if (u8_ProductID == IAM_TRIAD)
                {
                    set_output_bit(CTRL_OUT_TRIAD_LEXSOL_PUMP, cmd->data[0] != 0);
                }
                else
                {
                    set_output_bit(CTRL_OUT_FD_SHELL_FREEZER, cmd->data[0] != 0);
                }
            }
            break;

        case CONTROL_DEFROST_RLY:
            if (ControlHardware_Handler(cmd->action, cmd->data) == 0)
            {
                s_ctrl_status.mode = CTRL_MODE_MANUAL;
                set_output_bit(CTRL_OUT_DEFROST, cmd->data[0] != 0);
            }
            break;

        case CONTROL_VACUUM_VENT_VALVE:
            if (ControlHardware_Handler(cmd->action, cmd->data) == 0)
            {
                s_ctrl_status.mode = CTRL_MODE_MANUAL;
                set_output_bit(CTRL_OUT_VAC_VENT_VALVE, cmd->data[0] != 0);
            }
            break;

        case CONTROL_MINI_CHAMBER_HEATER:
            if (cmd->data_len >= sizeof(fd_ctrl_mini_chamber_payload_t))
            {
                fd_ctrl_mini_chamber_payload_t mc;
                memcpy(&mc, cmd->data, sizeof(mc));
                if (ControlHardware_Handler(cmd->action, cmd->data) == 0)
                {
                    s_ctrl_status.mode = CTRL_MODE_MANUAL;
                    set_output_bit(CTRL_OUT_FD_MINI_HEATER, mc.on_off != 0);
                    s_ctrl_status.mini_chamber_target_dC = mc.target_temp_dC;
                }
            }
            break;

        case CONTROL_MANUAL_SHELF_HEAT:
            if (cmd->data_len >= sizeof(fd_ctrl_manual_shelf_payload_t))
            {
                fd_ctrl_manual_shelf_payload_t shelf;
                memcpy(&shelf, cmd->data, sizeof(shelf));
                if (ControlHardware_Handler(cmd->action, cmd->data) == 0)
                {
                    s_ctrl_status.mode = CTRL_MODE_MANUAL;
                    set_output_bit(CTRL_OUT_TRIAD_SHELF_HEAT, (shelf.flags & CTRL_SHELF_ON_OFF_MASK) != 0);
                    s_ctrl_status.shelf_heat_flags = shelf.flags;
                    s_ctrl_status.shelf_target_dC  = shelf.target_temp_dC;
                }
            }
            break;

        case CONTROL_AUTO_MODE:
            s_ctrl_status.mode = (cmd->data[0] != 0) ? CTRL_MODE_AUTO : CTRL_MODE_OFF;
            break;

        case CONTROL_PROGRAM_RUN:
            s_ctrl_status.mode = (cmd->data[0] != 0) ? CTRL_MODE_PROGRAM : CTRL_MODE_OFF;
            break;

        default:
            break;
    }
}

/* =========================================================
 * PROCESS CONTROL SUB-MACHINES
 * ========================================================= */
static void collector_smf_run(void)
{
    /* Collector, Defrost & -105C Compressors */
}

static void vacuum_smf_run(void)
{
    /* Vacuum Pump, Purge & Vent Valve PID */
}

static void lexsol_smf_run(void)
{
    /* TRIAD Lexsol Pump, Pre-Freeze Ref Valve & Shelf Heater */
}

static void mode_smf_run(void)
{
    /* Auto-Start, Manual & Program Modes + End-Point Detection */
}

/* =========================================================
 * THREAD INIT & 100 MS RUNNER (1_Control_Thread)
 * ========================================================= */
int control_task_init(void)
{
    fd_control_hardware_init();
    s_ctrl_status.timestamp_ms = k_uptime_get_32();
    s_ctrl_status.valid        = true;
    zbus_chan_pub(&ZBUS_CHAN_CONTROL_STATUS, &s_ctrl_status, K_NO_WAIT);
    LOG_INF("1_Control_Thread initialized");
    return 0;
}

void control_task_run(void)
{
    struct sys_control_status_msg prev_status = s_ctrl_status;

    /* 1. Drain UI commands */
    struct ctrl_cmd_msg cmd;
    while (k_msgq_get(&ctrl_cmd_msgq, &cmd, K_NO_WAIT) == 0)
    {
        apply_ui_command(&cmd);
    }

    /* 2. Read latest system temperatures */
    struct sys_temp_msg temp_msg;
    if (zbus_chan_read(&ZBUS_CHAN_SYSTEM_TEMPERATURE, &temp_msg, K_NO_WAIT) == 0 && temp_msg.valid)
    {
        /* Available for collector_smf_run(), vacuum_smf_run(), lexsol_smf_run() */
    }

    /* 3. Common Process Controllers (FD & TRIAD) */
    collector_smf_run();
    vacuum_smf_run();
    mode_smf_run();

    /* 4. TRIAD-Only Process Controller */
    if (u8_ProductID == IAM_TRIAD)
    {
        lexsol_smf_run();
    }

    /* 5. Publish to ZBUS only when control status changes */
    if (memcmp(&prev_status, &s_ctrl_status, offsetof(struct sys_control_status_msg, timestamp_ms)) != 0)
    {
        s_ctrl_status.timestamp_ms = k_uptime_get_32();
        s_ctrl_status.valid        = true;
        zbus_chan_pub(&ZBUS_CHAN_CONTROL_STATUS, &s_ctrl_status, K_NO_WAIT);
        LOG_INF("[ZBUS] Control Status Updated: mode=%u, outputs=0x%04X",
                s_ctrl_status.mode, s_ctrl_status.output_flags);
    }
}
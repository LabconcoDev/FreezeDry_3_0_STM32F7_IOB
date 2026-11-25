#include "parser.h"
#include "sensor_handler.h"
#include "protocol_defs.h"
#include "DataStorage/accessory_iob_data.h"
#include "DataStorage/main_iob_data.h"
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <string.h>

LOG_MODULE_REGISTER(sensor_handler, LOG_LEVEL_INF);

extern void fd_protocol_send_dma(const uint8_t *data, size_t len);
extern struct k_mutex shadow_memory_mutex;
extern uint8_t u8_ProductID;

static void push_sensor_data(uint8_t action, const uint8_t *data, size_t data_len);
static void push_temp_sensor(uint8_t action);
static void push_vac_sensor(uint8_t action);
static void push_accy_ac_power(void);
static void push_hw_detection_iob(void);
static void push_hw_detection_accy(void);



static bool publish_accy_temp_zbus(uint8_t action, const uint8_t *data, size_t data_len)
{
    static struct accy_temp_msg accy_temps =
    {
        .shelf_dC     = { 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF },
        .sample_dC    = { 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF },
        .st_lexsol_dC = 0x7FFF,
        .timestamp_ms = 0,
        .valid        = false,
    };

    if (data_len < 2)
    {
        return false;
    }

    int16_t val = (int16_t)((uint16_t)data[0] | ((uint16_t)data[1] << 8));

    if (action == SENSOR_LEXSOL_TEMP)
    {
        accy_temps.st_lexsol_dC = val;
    }
    else if (action >= SENSOR_SHELF1_TEMP && action <= SENSOR_SHELF5_TEMP)
    {
        accy_temps.shelf_dC[action - SENSOR_SHELF1_TEMP] = val;
    }
    else if (action >= SENSOR_SHELF1_SAMPLE_TEMP && action <= SENSOR_SHELF5_SAMPLE_TEMP)
    {
        accy_temps.sample_dC[action - SENSOR_SHELF1_SAMPLE_TEMP] = val;
    }
    else
    {
        return false;
    }

    accy_temps.timestamp_ms = k_uptime_get_32();
    accy_temps.valid        = true;
    zbus_chan_pub(&ZBUS_CHAN_ACCESSORY_TEMPERATURE, &accy_temps, K_MSEC(5));
    return true;
}



void fd_sensor_handler_process(fd_interface_t source_if, uint8_t cls, uint8_t action, const uint8_t *data, size_t data_len)
{
    if (cls == CLASS_WRITE)
    {
        LOG_WRN("TYPE_SENSORS cannot have CLASS_WRITE (action=0x%02X)", action);
        return;
    }

    if (source_if == INTERFACE_ACCESSORY_IOB && cls == CLASS_PUSH)
    {
        if (!publish_accy_temp_zbus(action, data, data_len))
        {
            /* Non-temp accessory sensors still use UpdateAccessoryData for now */
            UpdateAccessoryData(action, data, data_len);
        }

        /* Relay live PUSH up to APP */
        push_sensor_data(action, data, data_len);
        return;
    }

    if (source_if == INTERFACE_APP && cls == CLASS_READ)
    {
        /* Reply directly from cached Main IOB / Accessory memory */
        PushSensorData(action);
    }
}

void PushSensorData(uint8_t action)
{
    switch (action)
    {
        case SENSOR_COLLECTOR_TEMP:
        case SENSOR_AMBIENT_TEMPERATURE:
        case SENSOR_LEXSOL_TEMP:
        case SENSOR_FIRST_STAGE_HEAT_EXCHANGER_OUTLET_TEMP:
        case SENSOR_SECOND_STAGE_HEAT_EXCHANGER_OUTLET_TEMP:
        case SENSOR_FIRST_STAGE_SUCTION_TEMP:
        case SENSOR_SECOND_STAGE_SUCTION_TEMP:
        case SENSOR_SHELL_FREEZER_TEMP_SENSOR:
        case SENSOR_MINI_CHAMBER_TEMP:
        case SENSOR_SHELF1_TEMP:
        case SENSOR_SHELF2_TEMP:
        case SENSOR_SHELF3_TEMP:
        case SENSOR_SHELF4_TEMP:
        case SENSOR_SHELF5_TEMP:
        case SENSOR_SHELF1_SAMPLE_TEMP:
        case SENSOR_SHELF2_SAMPLE_TEMP:
        case SENSOR_SHELF3_SAMPLE_TEMP:
        case SENSOR_SHELF4_SAMPLE_TEMP:
        case SENSOR_SHELF5_SAMPLE_TEMP:
            push_temp_sensor(action);
            break;

        case SENSOR_SYSTEM_VAC:
        case SENSOR_SAMPLE_VAC1:
        case SENSOR_SAMPLE_VAC2:
        case SENSOR_SAMPLE_VAC3:
        case SENSOR_SAMPLE_VAC4:
        case SENSOR_SAMPLE_VAC5:
        case SENSOR_SAMPLE_VAC6:
        case SENSOR_STOPPERING_TRAY_VACUUM:
        case SENSOR_VACUUM_END_POINT_ADDON1:
        case SENSOR_VACUUM_END_POINT_ADDON2:
            push_vac_sensor(action);
            break;

        case SENSOR_DRYING_ACCESSORY_AC_POWER:
            push_accy_ac_power();
            break;

        case SENSOR_HARDWARE_DETECTION_IOB:
            push_hw_detection_iob();
            break;

        case SENSOR_HARDWARE_SENSOR_DETECTION_ACCY:
            push_hw_detection_accy();
            break;

        default:
            LOG_WRN("Unhandled SENSOR action: 0x%02X", action);
            break;
    }
}

static void push_temp_sensor(uint8_t action)
{
    int16_t val = 0x7FFF; /* Default error/stale value */
    uint8_t out_data[2] = { 0xFF, 0x7F };

    /* 1. Check if this is an Accessory temperature (on FD) */
    bool is_accy_temp = (action >= SENSOR_SHELF2_TEMP && action <= SENSOR_SHELF5_TEMP) ||
                        (action >= SENSOR_SHELF2_SAMPLE_TEMP && action <= SENSOR_SHELF5_SAMPLE_TEMP) ||
                        (u8_ProductID != IAM_TRIAD &&
                         (action == SENSOR_LEXSOL_TEMP ||
                          action == SENSOR_SHELF1_TEMP ||
                          action == SENSOR_SHELF1_SAMPLE_TEMP));

    if (is_accy_temp)
    {
        struct accy_temp_msg accy_msg;
        if (zbus_chan_read(&ZBUS_CHAN_ACCESSORY_TEMPERATURE, &accy_msg, K_MSEC(5)) == 0 &&
            accy_msg.valid &&
            (k_uptime_get_32() - accy_msg.timestamp_ms) < 5000)
        {
            if (action == SENSOR_LEXSOL_TEMP)
            {
                val = accy_msg.st_lexsol_dC;
            }
            else if (action >= SENSOR_SHELF1_TEMP && action <= SENSOR_SHELF5_TEMP)
            {
                val = accy_msg.shelf_dC[action - SENSOR_SHELF1_TEMP];
            }
            else if (action >= SENSOR_SHELF1_SAMPLE_TEMP && action <= SENSOR_SHELF5_SAMPLE_TEMP)
            {
                val = accy_msg.sample_dC[action - SENSOR_SHELF1_SAMPLE_TEMP];
            }
        }

        out_data[0] = (uint8_t)(val & 0xFF);
        out_data[1] = (uint8_t)((val >> 8) & 0xFF);
        push_sensor_data(action, out_data, 2);
        return;
    }

    /* 2. Main IOB temperature: read directly from ZBUS_CHAN_SYSTEM_TEMPERATURE */
    struct sys_temp_msg temp_msg;
    if (zbus_chan_read(&ZBUS_CHAN_SYSTEM_TEMPERATURE, &temp_msg, K_MSEC(5)) == 0 &&
        temp_msg.valid &&
        (k_uptime_get_32() - temp_msg.timestamp_ms) < 5000)
    {
        switch (action)
        {
            case SENSOR_COLLECTOR_TEMP:                          val = temp_msg.collector_dC; break;
            case SENSOR_AMBIENT_TEMPERATURE:                     val = temp_msg.ambient_dC; break;
            case SENSOR_FIRST_STAGE_HEAT_EXCHANGER_OUTLET_TEMP:  val = temp_msg.hx_1st_stage_dC; break;
            case SENSOR_SECOND_STAGE_HEAT_EXCHANGER_OUTLET_TEMP: val = temp_msg.hx_2nd_stage_dC; break;
            case SENSOR_FIRST_STAGE_SUCTION_TEMP:                val = temp_msg.suction_1st_stage_dC; break;
            case SENSOR_SECOND_STAGE_SUCTION_TEMP:               val = temp_msg.suction_2nd_stage_dC; break;
            case SENSOR_SHELL_FREEZER_TEMP_SENSOR:
            case SENSOR_MINI_CHAMBER_TEMP:                       val = temp_msg.shell_or_mini_dC; break;
            case SENSOR_LEXSOL_TEMP:                             val = temp_msg.triad_lexsol_dC; break;
            case SENSOR_SHELF1_TEMP:                             val = temp_msg.triad_shelf_dC; break;
            case SENSOR_SHELF1_SAMPLE_TEMP:                      val = temp_msg.triad_sample_dC; break;
            default: break;
        }
    }

    out_data[0] = (uint8_t)(val & 0xFF);
    out_data[1] = (uint8_t)((val >> 8) & 0xFF);
    push_sensor_data(action, out_data, 2);
}

static void push_vac_sensor(uint8_t action)
{
    /* 2 bytes (uint16_t, mbar * 1000, LSB first) per ICD */
    uint8_t out_data[2] = { 0xFF, 0xFF };
    const uint8_t *raw = NULL;
    uint32_t ts = 0;

    k_mutex_lock(&shadow_memory_mutex, K_FOREVER);

    if (action == SENSOR_STOPPERING_TRAY_VACUUM)
    {
        ts  = AccessoryShadowMemory.Last_Update_ms;
        raw = AccessoryShadowMemory.DryerVacuum.raw;
    }
    else if (u8_ProductID == IAM_TRIAD)
    {
        ts = MainIOBShadowMemory.TRIAD.Vacuum_Sensors.Last_Update_ms;
        if (action == SENSOR_SYSTEM_VAC)
        {
            raw = MainIOBShadowMemory.TRIAD.Vacuum_Sensors.SystemVacuum.raw;
        }
        else if (action >= SENSOR_SAMPLE_VAC1 && action <= SENSOR_SAMPLE_VAC6)
        {
            raw = MainIOBShadowMemory.TRIAD.Vacuum_Sensors.SampleVacuum[action - SENSOR_SAMPLE_VAC1].raw;
        }
        else if (action == SENSOR_VACUUM_END_POINT_ADDON1 || action == SENSOR_VACUUM_END_POINT_ADDON2)
        {
            raw = MainIOBShadowMemory.TRIAD.Vacuum_Sensors.EndPointAddonVacuum[action - SENSOR_VACUUM_END_POINT_ADDON1].raw;
        }
    }
    else if (u8_ProductID == IAM_FD)
    {
        ts = MainIOBShadowMemory.FD.Vacuum_Sensors.Last_Update_ms;
        if (action == SENSOR_SYSTEM_VAC)
        {
            raw = MainIOBShadowMemory.FD.Vacuum_Sensors.SystemVacuum.raw;
        }
        else if (action >= SENSOR_SAMPLE_VAC1 && action <= SENSOR_SAMPLE_VAC6)
        {
            raw = MainIOBShadowMemory.FD.Vacuum_Sensors.SampleVacuum[action - SENSOR_SAMPLE_VAC1].raw;
        }
        else if (action == SENSOR_VACUUM_END_POINT_ADDON1 || action == SENSOR_VACUUM_END_POINT_ADDON2)
        {
            raw = MainIOBShadowMemory.FD.Vacuum_Sensors.EndPointAddonVacuum[action - SENSOR_VACUUM_END_POINT_ADDON1].raw;
        }
    }

    if (raw != NULL && (k_uptime_get_32() - ts) < 5000)
    {
        memcpy(out_data, raw, 2);
    }

    k_mutex_unlock(&shadow_memory_mutex);

    push_sensor_data(action, out_data, 2);
}

static void push_accy_ac_power(void)
{
    uint8_t val = 0x00;

    k_mutex_lock(&shadow_memory_mutex, K_FOREVER);
    val = AccessoryShadowMemory.AcPowerOn;
    k_mutex_unlock(&shadow_memory_mutex);

    push_sensor_data(SENSOR_DRYING_ACCESSORY_AC_POWER, &val, 1);
}

static void push_hw_detection_iob(void)
{
    fd_hw_detect_iob_payload_t hw = { 0 };

    k_mutex_lock(&shadow_memory_mutex, K_FOREVER);
    if (u8_ProductID == IAM_TRIAD)
    {
        hw = MainIOBShadowMemory.TRIAD.Digital_Sensors.HardwareDetect;
    }
    else if (u8_ProductID == IAM_FD)
    {
        hw = MainIOBShadowMemory.FD.Digital_Sensors.HardwareDetect;
    }
    k_mutex_unlock(&shadow_memory_mutex);

    push_sensor_data(SENSOR_HARDWARE_DETECTION_IOB, (const uint8_t *)&hw, sizeof(hw));
}

static void push_hw_detection_accy(void)
{
    fd_hw_detect_accy_payload_t hw = { 0 };

    k_mutex_lock(&shadow_memory_mutex, K_FOREVER);
    hw = AccessoryShadowMemory.HardwareDetect;
    k_mutex_unlock(&shadow_memory_mutex);

    push_sensor_data(SENSOR_HARDWARE_SENSOR_DETECTION_ACCY, (const uint8_t *)&hw, sizeof(hw));
}

static void push_sensor_data(uint8_t action, const uint8_t *data, size_t data_len)
{
    uint8_t tx_buf[32];
    size_t frame_len = fd_serializer_build_frame(tx_buf, sizeof(tx_buf), CLASS_PUSH, TYPE_SENSORS, action, data, data_len);
    if (frame_len > 0)
    {
        fd_protocol_send_dma(tx_buf, frame_len);
    }
    else
    {
        LOG_ERR("Failed to serialize PUSH packet (action 0x%02X)", action);
    }
}
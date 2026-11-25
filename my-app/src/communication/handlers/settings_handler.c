#include "settings_handler.h"
#include "serializer.h"
#include "product_type.h"
#include <string.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(settings_handler, LOG_LEVEL_INF);

extern void fd_protocol_send_dma(const uint8_t *data, size_t len);
extern void fd_protocol_send_accessory_iob_dma(const uint8_t *data, size_t len);
extern uint8_t u8_ProductID;

/* Temporary RAM storage until NVS/Flash persistence is wired in Step 5 */
static int16_t temp_offsets[15] = { 0 }; /* Index 1..14 */
static int16_t vac_offsets[9]   = { 0 }; /* Index 1..8  */

static void send_settings_frame(fd_interface_t target_if, uint8_t cls, uint8_t action, const uint8_t *data, size_t data_len)
{
    uint8_t tx_buf[32];
    size_t frame_len = fd_serializer_build_frame(tx_buf, sizeof(tx_buf), cls, TYPE_SETTINGS, action, data, data_len);

    if (frame_len == 0)
    {
        LOG_ERR("Failed to serialize SETTINGS frame (action 0x%02X)", action);
        return;
    }

    if (target_if == INTERFACE_APP)
    {
        fd_protocol_send_dma(tx_buf, frame_len);
    }
    else
    {
        fd_protocol_send_accessory_iob_dma(tx_buf, frame_len);
    }
}

static bool should_forward_calib_to_accy(uint8_t action, uint8_t target_sensor)
{
    if (action == SETTING_CALIB_VACUUM_OFFSET)
    {
        return (target_sensor == CALIB_VAC_ST_DRYER);
    }

    if (action == SETTING_CALIB_TEMP_OFFSET)
    {
        if (u8_ProductID != IAM_TRIAD)
        {
            return (target_sensor == CALIB_TEMP_LEXSOL || target_sensor >= CALIB_TEMP_SAMPLE_1);
        }
    }

    return false;
}

void fd_settings_handler_process(fd_interface_t source_if, uint8_t cls, uint8_t action, const uint8_t *data, size_t data_len)
{
    /* Forward Accessory PUSH replies up to APP */
    if (source_if == INTERFACE_ACCESSORY_IOB && cls == CLASS_PUSH)
    {
        send_settings_frame(INTERFACE_APP, CLASS_PUSH, action, data, data_len);
        return;
    }

    if (source_if != INTERFACE_APP)
    {
        return;
    }

    switch (action)
    {
        case SETTING_CALIB_TEMP_OFFSET:
        case SETTING_CALIB_VACUUM_OFFSET:
            if (data_len < 1)
            {
                return;
            }

            if (should_forward_calib_to_accy(action, data[0]))
            {
                send_settings_frame(INTERFACE_ACCESSORY_IOB, cls, action, data, data_len);
                return;
            }

            if (cls == CLASS_WRITE && data_len >= sizeof(fd_calib_offset_payload_t))
            {
                fd_calib_offset_payload_t calib;
                memcpy(&calib, data, sizeof(calib));

                if (action == SETTING_CALIB_TEMP_OFFSET && calib.target_sensor >= 1 && calib.target_sensor <= 14)
                {
                    temp_offsets[calib.target_sensor] = calib.offset;
                }
                else if (action == SETTING_CALIB_VACUUM_OFFSET && calib.target_sensor >= 1 && calib.target_sensor <= 8)
                {
                    vac_offsets[calib.target_sensor] = calib.offset;
                }

                LOG_INF("Calib WRITE action=0x%02X sensor=%u offset=%d", action, calib.target_sensor, calib.offset);
                /* TODO (Step 5): Persist to NVS/Flash and apply in task_sensor */
            }
            else if (cls == CLASS_READ && data_len >= 1)
            {
                fd_calib_offset_payload_t resp = { 0 };
                resp.target_sensor = data[0];

                if (action == SETTING_CALIB_TEMP_OFFSET && resp.target_sensor >= 1 && resp.target_sensor <= 14)
                {
                    resp.offset = temp_offsets[resp.target_sensor];
                }
                else if (action == SETTING_CALIB_VACUUM_OFFSET && resp.target_sensor >= 1 && resp.target_sensor <= 8)
                {
                    resp.offset = vac_offsets[resp.target_sensor];
                }

                send_settings_frame(INTERFACE_APP, CLASS_PUSH, action, (const uint8_t *)&resp, sizeof(resp));
            }
            break;

        default:
            LOG_WRN("Unknown SETTINGS action: 0x%02X", action);
            break;
    }
}
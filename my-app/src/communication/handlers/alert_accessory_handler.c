#include "alert_accessory_handler.h"
#include "serializer.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(alert_handler, LOG_LEVEL_INF);

extern void fd_protocol_send_dma(const uint8_t *data, size_t len);
extern void fd_protocol_send_accessory_iob_dma(const uint8_t *data, size_t len);

static void forward_frame(fd_interface_t target_if, uint8_t cls, uint8_t action, const uint8_t *data, size_t data_len)
{
    uint8_t tx_buf[128];
    size_t frame_len = fd_serializer_build_frame(tx_buf, sizeof(tx_buf), cls, TYPE_ALERT, action, data, data_len);

    if (frame_len == 0)
    {
        LOG_ERR("Failed to serialize ALERT frame (action 0x%02X)", action);
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

void fd_alert_handler_process(fd_interface_t source_if, uint8_t cls, uint8_t action, const uint8_t *data, size_t data_len)
{
    if (source_if == INTERFACE_ACCESSORY_IOB && cls == CLASS_PUSH)
    {
        /* Forward accessory alerts up to the APP */
        LOG_INF("Forwarding Accessory Alert 0x%02X to APP", action);
        forward_frame(INTERFACE_APP, CLASS_PUSH, action, data, data_len);
        return;
    }

    switch (action)
    {
        case ALERT_ACK_BY_USER_CLEAR_ALL:
            if (cls == CLASS_PUSH && source_if == INTERFACE_APP)
            {
                LOG_INF("Clearing all active alarms");
                /* TODO: Clear Main IOB alarms in thread_alerts / DataStorage */
                forward_frame(INTERFACE_ACCESSORY_IOB, CLASS_PUSH, action, NULL, 0);
            }
            break;

        case ALERT_CLEAR_SPECIFIC_ALARM:
            if (cls == CLASS_PUSH && source_if == INTERFACE_APP && data_len >= 1)
            {
                LOG_INF("Clearing specific alarm ID: 0x%02X", data[0]);
                /* TODO: Clear alarm data[0] on Main IOB */
                forward_frame(INTERFACE_ACCESSORY_IOB, CLASS_PUSH, action, data, 1);
            }
            break;

        default:
            LOG_WRN("Unhandled ALERT action: 0x%02X (cls: 0x%02X)", action, cls);
            break;
    }
}
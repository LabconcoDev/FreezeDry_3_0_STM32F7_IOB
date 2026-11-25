#include "diagnostic_handler.h"
#include "serializer.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(diagnostic_handler, LOG_LEVEL_INF);

extern void fd_protocol_send_dma(const uint8_t *data, size_t len);

void fd_diagnostic_handler_process(fd_interface_t source_if, uint8_t cls, uint8_t action, const uint8_t *data, size_t data_len)
{
    if (source_if != INTERFACE_APP)
    {
        return;
    }

    switch (action)
    {
        case DIAGNOSTIC_START:
            if (cls == CLASS_WRITE && data_len >= 1)
            {
                LOG_INF("Diagnostic START requested for Test ID: %u", data[0]);
                /* TODO (Step 5): Trigger non-blocking diagnostic state machine in task_control */
            }
            break;

        case DIAGNOSTIC_STOP:
            if (cls == CLASS_WRITE && data_len >= 1)
            {
                LOG_INF("Diagnostic STOP requested for Test ID: %u", data[0]);
                /* TODO (Step 5): Abort active diagnostic and turn off all loads */
            }
            break;

        default:
            LOG_WRN("Unhandled DIAGNOSTIC action: 0x%02X", action);
            break;
    }
}
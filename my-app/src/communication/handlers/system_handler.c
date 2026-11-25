#include "parser.h"
#include "system_handler.h"
#include "product_type.h"
#include "protocol_defs.h"
#include "serializer.h"
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(system_handler, LOG_LEVEL_INF);

extern void fd_protocol_send_dma(const uint8_t *data, size_t len);
extern void fd_protocol_send_accessory_iob_dma(const uint8_t *data, size_t len);

static void send_system_frame(fd_interface_t target_if, uint8_t cls, uint8_t action, const uint8_t *data, size_t data_len);
static void push_iob_firmware_version(void);
static void push_iob_model_details(void);
static void push_system_date(void);
static void push_system_time(void);

void fd_system_handler_process(fd_interface_t source_if, uint8_t cls, uint8_t action, const uint8_t *data, size_t data_len)
{
    /* Forward Accessory PUSH messages up to APP */
    if (source_if == INTERFACE_ACCESSORY_IOB && cls == CLASS_PUSH)
    {
        if (action == SYSTEM_MODEL_DETAILS_ACCY && data_len >= sizeof(fd_model_details_payload_t))
        {
            LOG_INF("Received Accessory Model Details from Accessory IOB");
            /* TODO: Update u8_AccessoryID from model details */
        }
        send_system_frame(INTERFACE_APP, CLASS_PUSH, action, data, data_len);
        return;
    }

    switch (action)
    {
        case SYSTEM_FIRMWARE_VERSION_IOB:
            if (cls == CLASS_READ)
            {
                push_iob_firmware_version();
            }
            break;

        case SYSTEM_MODEL_DETAILS_IOB:
            if (cls == CLASS_READ)
            {
                push_iob_model_details();
            }
            else if (cls == CLASS_WRITE && data_len >= sizeof(fd_model_details_payload_t))
            {
                fd_model_details_payload_t details;
                memcpy(&details, data, sizeof(details));
                LOG_INF("Write Main IOB Model Details: %.4s-%.4s-%.4s",
                        details.model, details.special, details.serial);
                /* TODO: Save to NVS / Flash and update u8_ProductID */
            }
            break;

        case SYSTEM_DATE:
            if (cls == CLASS_READ)
            {
                push_system_date();
            }
            else if (cls == CLASS_WRITE && data_len >= sizeof(fd_date_payload_t))
            {
                fd_date_payload_t date;
                memcpy(&date, data, sizeof(date));
                LOG_INF("Set Date: %02u/%02u/20%02u", date.day, date.month, date.year);
                /* TODO: Update RTC date */
            }
            break;

        case SYSTEM_TIME:
            if (cls == CLASS_READ)
            {
                push_system_time();
            }
            else if (cls == CLASS_WRITE && data_len >= sizeof(fd_time_payload_t))
            {
                fd_time_payload_t time;
                memcpy(&time, data, sizeof(time));
                LOG_INF("Set Time: %02u:%02u:%02u", time.hours, time.minutes, time.seconds);
                /* TODO: Update RTC time */
            }
            break;

        case SYSTEM_I_AM_ALIVE:
            if (cls == CLASS_WRITE)
            {
                LOG_INF("Received SYSTEM_I_AM_ALIVE from APP");
                /* TODO: Reset APP heartbeat / watchdog timer if used */
            }
            break;

        case SYSTEM_FIRMWARE_VERSION_ACCY:
        case SYSTEM_MODEL_DETAILS_ACCY:
            /* Bridge READ / WRITE from APP down to Accessory IOB */
            if (source_if == INTERFACE_APP && (cls == CLASS_READ || cls == CLASS_WRITE))
            {
                send_system_frame(INTERFACE_ACCESSORY_IOB, cls, action, data, data_len);
            }
            break;

        case SYSTEM_FIRMWARE_UPDATE_REQUEST_ACCY:
            if (source_if == INTERFACE_APP && cls == CLASS_WRITE)
            {
                LOG_INF("Accessory firmware update requested");
                /* TODO: Trigger Main IOB -> Accessory IOB bootloader update */
            }
            break;

        case SYSTEM_DEBUG_LOG:
        case SYSTEM_I_AM_DONE:
        case SYSTEM_PING:
        case SYSTEM_TERMINAL:
        case SYSTEM_FIRMWARE_UPDATE_RESULT_ACCY:
        case SYSTEM_FIRMWARE_UPDATE_PROGRESS_ACCY:
            /* Outgoing PUSH-only messages */
            break;

        default:
            LOG_WRN("Unknown SYSTEM action: 0x%02X", action);
            break;
    }
}

void ReadAccessoryType(void)
{
    /* Query Accessory IOB for its model details over Accessory UART */
    send_system_frame(INTERFACE_ACCESSORY_IOB, CLASS_READ, SYSTEM_MODEL_DETAILS_ACCY, NULL, 0);
    LOG_INF("Sent READ request for Accessory Model Details");
}

void pushIamAlive(void)
{
    send_system_frame(INTERFACE_APP, CLASS_PUSH, SYSTEM_I_AM_ALIVE, NULL, 0);
}

void pushIamDone(void)
{
    send_system_frame(INTERFACE_APP, CLASS_PUSH, SYSTEM_I_AM_DONE, NULL, 0);
}

static void push_iob_firmware_version(void)
{
    /* Byte 0: Major (char), Byte 1: Minor (char), Byte 2: Micro (uint8_t), Byte 3-n: Product No */
    const uint8_t ver[] = { 'A', '0', 1, 'F', 'D', '3', '0' };
    send_system_frame(INTERFACE_APP, CLASS_PUSH, SYSTEM_FIRMWARE_VERSION_IOB, ver, sizeof(ver));
}

static void push_iob_model_details(void)
{
    fd_model_details_payload_t details = { 0 };
    /* TODO: Read stored model, special, serial from NVS / Flash */
    send_system_frame(INTERFACE_APP, CLASS_PUSH, SYSTEM_MODEL_DETAILS_IOB, (const uint8_t *)&details, sizeof(details));
}

static void push_system_date(void)
{
    fd_date_payload_t date = { 0 };
    /* TODO: Read from RTC */
    send_system_frame(INTERFACE_APP, CLASS_PUSH, SYSTEM_DATE, (const uint8_t *)&date, sizeof(date));
}

static void push_system_time(void)
{
    fd_time_payload_t time = { 0 };
    /* TODO: Read from RTC */
    send_system_frame(INTERFACE_APP, CLASS_PUSH, SYSTEM_TIME, (const uint8_t *)&time, sizeof(time));
}

static void send_system_frame(fd_interface_t target_if, uint8_t cls, uint8_t action, const uint8_t *data, size_t data_len)
{
    uint8_t tx_buf[128];
    size_t frame_len = fd_serializer_build_frame(tx_buf, sizeof(tx_buf), cls, TYPE_SYSTEM, action, data, data_len);

    if (frame_len == 0)
    {
        LOG_ERR("Failed to serialize SYSTEM frame (action 0x%02X)", action);
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
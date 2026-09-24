#include "parser.h"
#include "control_handler.h"
#include "sensor_handler.h"
#include "system_handler.h"
#include <zephyr/logging/log.h>
#include "serializer.h"


LOG_MODULE_REGISTER(protocol_parser, LOG_LEVEL_INF);



/* External link to the transmit function in task_dma_uart_display.c */
extern void fd_protocol_send_dma(const uint8_t *data, size_t len);
extern void fd_protocol_send_accessory_iob_dma(const uint8_t *data, size_t len);


void fd_parser_process_packet(fd_interface_t source_if, const fd_packet_t *pkt) 
{
    if (pkt == NULL || pkt->payload == NULL || pkt->len < 3) {
        return; // Invalid payload
    }

    fd_payload_t *payload = pkt->payload;
    uint8_t data_len = pkt->len - 3; // Subtract cls, type, action bytes

    /* ======================================================== */
    /*                  NEW AUTO-ACK LOGIC                      */
    /* ======================================================== */
    /* Only ACK incoming READ or WRITE commands to prevent loops */
    if (payload->cls == CLASS_WRITE || payload->cls == CLASS_READ) {
        
        uint8_t tx_buf[128]; // Temporary buffer for the outgoing frame
        
        /* Re-package the exact same Type, Action, and Data, but change Class to ACK */
        size_t frame_len = fd_serializer_build_frame(tx_buf, sizeof(tx_buf),
                                                     CLASS_ACK, 
                                                     payload->type, 
                                                     payload->action, 
                                                     payload->data, 
                                                     data_len);
        
        /* If serialization succeeded, send it via DMA */
        if (frame_len > 0) 
        {



            if (source_if == INTERFACE_APP) 
            {
                fd_protocol_send_dma(tx_buf, frame_len);
                LOG_INF("To Display Auto-ACK sent for Type: 0x%02X, Action: 0x%02X", payload->type, payload->action);
            }
            else 
            {
                fd_protocol_send_accessory_iob_dma(tx_buf, frame_len);
                LOG_INF("To Accesssory IOB Auto-ACK sent for Type: 0x%02X, Action: 0x%02X", payload->type, payload->action);
            }
        } 
        else 
        {
            LOG_ERR("Failed to serialize Auto-ACK (buffer too small)");
        }
    }
    /* ======================================================== */


    switch (payload->type) {
        case TYPE_CONTROL:
            fd_control_handler_process(payload->cls, payload->action, payload->data, data_len);
            break;
            

        case TYPE_SENSORS:
            fd_sensor_handler_process(payload->cls, payload->action, payload->data, data_len);
            break;
        case TYPE_ALERT:
        case TYPE_SYSTEM:
            fd_system_handler_process(payload->cls, payload->action, payload->data, data_len);
            break;
            

        case TYPE_SETTINGS:
        case TYPE_DIAGNOSTIC:
            // TODO: Implement handlers for other types later
            LOG_INF("Received unhandled type: 0x%02X", payload->type);
            break;
            
        default:
            LOG_WRN("Unknown packet type: 0x%02X", payload->type);
            break;
    }
}
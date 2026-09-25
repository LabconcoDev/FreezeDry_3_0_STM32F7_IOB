/*
 * control_accessory_handler.c
 *
 * Implementation of the accessory Gatekeeper and UART forwarder.
 */

#include "control_accessory_handler.h"
#include "protocol_defs.h"
#include "serializer.h"
#include "product_type.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(accessory_control, LOG_LEVEL_INF);

/* External link to your UART DMA transmission function */
extern void fd_protocol_send_accessory_iob_dma(const uint8_t *data, size_t len);

/* 
 * The Gatekeeper / Forwarder Function
 */
void accessory_control_handler_process(uint8_t action, const uint8_t *data, size_t data_len) {
    
    // 1. GATEKEEPER VALIDATION
    // Check if there is actually an accessory attached to the system.
    uint8_t current_accessory = FindAccessoryType(); // grab the data from EEPROM / variable
    
    if (current_accessory == IAM_UNKNOWN ) 
    {
        // Let APP know its trying to send to something thats not there
        LOG_WRN("Ignored Action 0x%02X: No accessory is currently attached.", action);
        return;
    }

    // 2. SERIALIZATION
    // We have a valid accessory. Reconstruct the protocol frame.
    uint8_t tx_buf[128]; // Ensure this is large enough for your maximum packet size
    
    size_t frame_len = fd_serializer_build_frame(tx_buf, 
                                                 sizeof(tx_buf),
                                                 CLASS_WRITE, 
                                                 TYPE_CONTROL, 
                                                 action, 
                                                 data, 
                                                 data_len);
    
    // 3. UART TRANSMISSION
    if (frame_len > 0) 
    {
        LOG_INF("Forwarding Action 0x%02X to Accessory IOB over UART.", action);
        fd_protocol_send_accessory_iob_dma(tx_buf, frame_len);
    } 
    else 
    {
        LOG_ERR("Failed to serialize frame for accessory forwarding.");
    }
}
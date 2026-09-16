#include "control_handler.h"
#include <zephyr/logging/log.h>
#include "serializer.h"



LOG_MODULE_REGISTER(control_handler, LOG_LEVEL_INF);

/* External link to the transmit function */
extern void fd_protocol_send_dma(const uint8_t *data, size_t len);
/* Forward declaration */
void pushRefrigRly(void);
void fd_control_handler_process(uint8_t cls, uint8_t action, const uint8_t *data, size_t data_len) {
    
    
   

    switch (action) {
        case CONTROL_REFRIG_RLY:
            if (cls==CLASS_READ)
            {
                pushRefrigRly(); // need to pass port as a parameter
            }
            else if (cls == CLASS_WRITE)
            {
                // write parameter to EEPROM or control the hardware
                if (data_len > 0) 
                {
                    LOG_INF("Setting Refrigeration Relay to %d", data[0]);
                    // TODO: Interface with hardware / GPIO driver here
                }
            }
            else
            {
                // Error code
            }
            break;

        case CONTROL_VACUUM_RLY:
            if (data_len > 0) {
                LOG_INF("Setting Vacuum Relay to %d", data[0]);
                // TODO: Interface with hardware / GPIO driver here
            }
            break;

        default:
            LOG_WRN("Unknown Control Action: 0x%02X", action);
            break;
    }
}

void pushRefrigRly(void)
{
    /* 1. Get the current hardware state */
    // TODO: Read your actual hardware GPIO or state variable here.
    // For this example, let's assume the relay is currently ON (0x01)
    uint8_t current_state = 0x01; 

    /* 2. Create a temporary buffer for the outgoing frame */
    uint8_t tx_buf[32]; 

    /* 3. Use the Serializer to build a PUSH packet */
    // Note: We use CLASS_PUSH to reply to a READ request, echoing the Type and Action.
    size_t frame_len = fd_serializer_build_frame(tx_buf, sizeof(tx_buf),
                                                 CLASS_PUSH,          // The reply class
                                                 TYPE_CONTROL,        // The Type
                                                 CONTROL_REFRIG_RLY,  // The Action
                                                 &current_state,      // The data we are sending back
                                                 1);                  // Length of the data (1 byte)
    
    /* 4. Send the packet via DMA */
    if (frame_len > 0) {
        fd_protocol_send_dma(tx_buf, frame_len);
        LOG_INF("Pushed Refrigeration Relay state: %d", current_state);
    } else {
        LOG_ERR("Failed to serialize PUSH packet");
    }
}
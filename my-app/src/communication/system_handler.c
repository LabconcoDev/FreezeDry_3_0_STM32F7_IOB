#include "product_type.h"
#include "protocol_defs.h"
#include <zephyr/logging/log.h>
#include "serializer.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(system_handler, LOG_LEVEL_INF);
/* External link to the transmit function */
extern void fd_protocol_send_dma(const uint8_t *data, size_t len);




void pushModelDetails(void);

void fd_system_handler_process(uint8_t cls, uint8_t action, const uint8_t *data, size_t data_len) {
    

    switch (action) 
    {
        case SYSTEM_MODEL_DETAILS:

            if (cls==CLASS_READ) // This is a read request from APP/2ndary IOB
            {
                pushModelDetails(); // need to pass port as a parameter
            }
            else if (cls == CLASS_WRITE) // This request should be from APP. will this come from 2ndary IOB?
            {
                // write parameter to EEPROM 
                if (data_len > 0) 
                {
                    // This might be 2 bytes of data
                    LOG_INF("Setting Model details to %d", data[0]);
                    
                }
            }
            /*Push messages from 2ndary IOB are informative msgs to main IOB*/
            else if (cls == CLASS_PUSH)
            {
                if (data_len > 0) 
                {
                    // This might be 2 bytes of data
                    LOG_INF("Model details of 2ndary IOB %d", data[0]);
                    // This push message should be send to APP. 
 
                }
            }
            break;


        

        default:
            // Error Code
            break;
    }

}
void ReadAccessoryType()
{

    uint8_t tx_buf[32]; 
    
    size_t frame_len = fd_serializer_build_frame(tx_buf, sizeof(tx_buf),
                                                 CLASS_READ,          // The reply class
                                                 TYPE_SYSTEM,  // The Action
                                                 SYSTEM_MODEL_DETAILS,// The data we are sending back
                                                 NULL,       // No data
                                                 0
                                                );                  // Length of the data (1 byte) 
    /*Send the packet via DMA */
    if (frame_len > 0) {
        fd_protocol_send_dma(tx_buf, frame_len);
        LOG_INF("Sent Read Request for Accesory Type");
    } else {
        LOG_ERR("Failed to serialize Read Request for Accessory Type packet");
    }

}

void pushModelDetails(void)
{
    // Implement 
}
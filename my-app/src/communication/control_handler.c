
/*

The control messages to accessory board need to be routed accordingly. 
The request for access board parameters to be handled differently than the template below.
Triad/FD would be same board, hence messages to be handled accordingly.
Synchronization while parameter to be fetched from the accessory board to be designed carefully!

*/


#include "control_accessory_handler.h"
#include "control_hardware_handler.h"
#include "control_handler.h"
#include <zephyr/logging/log.h>
#include "serializer.h"
#include "product_type.h"


LOG_MODULE_REGISTER(control_handler, LOG_LEVEL_INF);

/* External link to the transmit function */
extern void fd_protocol_send_dma(const uint8_t *data, size_t len);

void pushRefrigRly(void);
void pushVacuumRly(void);
void pushPurgeRly(void);
void pushShellRly(void);
void pushRefrigDlyRly(void);
void pushVentValve(void);
void pushDefrostRly(void);
void pushMiniChamberHtr(void);
void pushShelfMstrHtr(void);
void pushShelf1Htr(void);
void pushShelf2Htr(void);
void pushShelf3Htr(void);
void pushShelf4Htr(void);
void pushShelf5Htr(void);
void pushLexsolPumpRly(void);
void pushLexsolHtr(void);
void pushRefValveRly(void);
void pushTiltValve(void);

void push5VFanDrv(void);


void fd_control_handler_process(uint8_t cls, uint8_t action, const uint8_t *data, size_t data_len) {
    

    switch (action) {

        /* -------------------------------------------------------- */
        /* CATEGORY 1: BASE UNIT ACTIONS (Local GPIO Only)                      */
        /* -------------------------------------------------------- */
        case CONTROL_VACUUM_RLY:
            if (cls == CLASS_READ) 
            {
                pushVacuumRly(); 
            } 
            else if (cls == CLASS_WRITE) 
            {
                ControlHardware_Handler(action, data);
            }
            break;

        case CONTROL_PURGE_RLY:
            if (cls==CLASS_READ)
            {
                pushPurgeRly(); // need to pass port as a parameter
            }
            else if (cls == CLASS_WRITE) 
            {
                ControlHardware_Handler(action, data);
            }
            break;


        case CONTROL_SECOND_STAGE_REFRIG_RLY  : // This is second stage compressor
            if (cls==CLASS_READ)
            {
                pushRefrigDlyRly(); // need to pass port as a parameter
            }
            else if (cls == CLASS_WRITE) 
            {
                ControlHardware_Handler(action, data);
            }
            break;

        case CONTROL_VENT_VALVE: 
            if (cls==CLASS_READ)
            {
                pushVentValve(); // need to pass port as a parameter
            }
            else if (cls == CLASS_WRITE) 
            {
                ControlHardware_Handler(action, data);
            }
            break;


        case CONTROL_DEFROST_RELAY: 
            if (cls==CLASS_READ)
            {
                pushDefrostRly(); // need to pass port as a parameter
            }
            else if (cls == CLASS_WRITE) 
            {
                ControlHardware_Handler(action, data);
            }
            break;


        case CONTROL_MINI_CHAMBER_HTR: 
            if (cls==CLASS_READ)
            {
                pushMiniChamberHtr(); // need to pass port as a parameter
            }
            else if (cls == CLASS_WRITE) 
            {
                ControlHardware_Handler(action, data);
            }
            break;

        case CONTROL_REF_VALVE_RLY: 
            if (cls==CLASS_READ)
            {
                pushRefValveRly(); // need to pass port as a parameter
            }
            else if (cls == CLASS_WRITE) 
            {
                ControlHardware_Handler(action, data);
            }
            break;


        case CONTROL_TILT_VALVE: 
            if (cls==CLASS_READ)
            {
                pushTiltValve(); // need to pass port as a parameter
            }
            else if (cls == CLASS_WRITE) 
            {
                ControlHardware_Handler(action, data);
            }
            break;

        case CONTROL_5V_FAN_DRV: 
            if (cls==CLASS_READ)
            {
                push5VFanDrv(); // need to pass port as a parameter
            }
            else if (cls == CLASS_WRITE) 
            {
                ControlHardware_Handler(action, data);
            }
            break;
          
        

        /* ======================================================== */
        /* CATEGORY 2: PURELY ACCESSORY ACTIONS (No ID checks needed!)  */
        /* ======================================================== */

        case CONTROL_SHELF_MSTR_HTR_CNTRL: 
            if (cls==CLASS_READ)
            {
                pushShelfMstrHtr(); // need to pass port as a parameter
            }
            else if (cls == CLASS_WRITE) 
            {
                accessory_control_handler_process(action, data, data_len);
            }
            break;

        case CONTROL_SHELF1_HTR: 
            if (cls==CLASS_READ)
            {
                pushShelf1Htr(); // need to pass port as a parameter
            }
            else if (cls == CLASS_WRITE) 
            {
                accessory_control_handler_process(action, data, data_len);
            }
            break;

        case CONTROL_SHELF2_HTR: 
            if (cls==CLASS_READ)
            {
                pushShelf2Htr(); // need to pass port as a parameter
            }
            else if (cls == CLASS_WRITE) 
            {
                accessory_control_handler_process(action, data, data_len);
            }
            break;

        case CONTROL_SHELF3_HTR: 
            if (cls==CLASS_READ)
            {
                pushShelf3Htr(); // need to pass port as a parameter
            }
            else if (cls == CLASS_WRITE) 
            {
                accessory_control_handler_process(action, data, data_len);
            }
            break;

        case CONTROL_SHELF4_HTR: 
            if (cls==CLASS_READ)
            {
                pushShelf4Htr(); // need to pass port as a parameter
            }
            else if (cls == CLASS_WRITE) 
            {
                accessory_control_handler_process(action, data, data_len);
            }
            break;


        case CONTROL_SHELF5_HTR: 
            if (cls==CLASS_READ)
            {
                pushShelf5Htr(); // need to pass port as a parameter
            }
            else if (cls == CLASS_WRITE) 
            {
                accessory_control_handler_process(action, data, data_len);
            }
            break;

        


        /* ======================================================== */
        /* CATEGORY 3: SHARED ACTIONS (Local AND Accessory)         */
        /* ======================================================== */

        case CONTROL_REFRIG_RLY:
            if (cls==CLASS_READ)
            {              
                pushRefrigRly(); // need to pass port as a parameter
            }
            else if (cls == CLASS_WRITE)
            {
                // 1. Tell accessory what to do
                accessory_control_handler_process(action, data, data_len);
                // 2. control action on main IOB
                ControlHardware_Handler(action, data);
            }
            
            break;
        
        case CONTROL_LEXSOL_PUMP_RLY: 
            if (cls==CLASS_READ)
            {
                pushLexsolPumpRly(); // need to pass port as a parameter
            }
            else if (cls == CLASS_WRITE)
            {
                accessory_control_handler_process(action, data, data_len);
                ControlHardware_Handler(action, data);
            }
            break;

        case CONTROL_LEXSOL_HTR: 
            if (cls==CLASS_READ)
            {
                pushLexsolHtr(); // need to pass port as a parameter
            }
            else if (cls == CLASS_WRITE)
            {
                accessory_control_handler_process(action, data, data_len);
                ControlHardware_Handler(action, data);
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
void pushVacuumRly(void)
{
    uint8_t current_state = 0x01; 
    uint8_t tx_buf[32]; 
    size_t frame_len = fd_serializer_build_frame(tx_buf, sizeof(tx_buf),
                                                 CLASS_PUSH,          // The reply class
                                                 TYPE_CONTROL,        // The Type
                                                 CONTROL_VACUUM_RLY,  // The Action
                                                 &current_state,      // The data we are sending back
                                                 1);                  // Length of the data (1 byte)
    if (frame_len > 0) 
    {
        fd_protocol_send_dma(tx_buf, frame_len);
        LOG_INF("Pushed Vacuum Relay state: %d", current_state);
    } 
    else 
    {
        LOG_ERR("Failed to serialize PUSH packet");
    }
}
void pushPurgeRly(void)
{
    uint8_t current_state = 0x01; 
    uint8_t tx_buf[32]; 
    size_t frame_len = fd_serializer_build_frame(tx_buf, sizeof(tx_buf),
                                                 CLASS_PUSH,          // The reply class
                                                 TYPE_CONTROL,        // The Type
                                                 CONTROL_PURGE_RLY,  // The Action
                                                 &current_state,      // The data we are sending back
                                                 1);                  // Length of the data (1 byte)
    if (frame_len > 0) 
    {
        fd_protocol_send_dma(tx_buf, frame_len);
        LOG_INF("Pushed Purge Relay state: %d", current_state);
    } 
    else 
    {
        LOG_ERR("Failed to serialize PUSH packet");
    }
}

void pushShellRly(void)
{
    uint8_t current_state = 0x01; 
    uint8_t tx_buf[32]; 
    size_t frame_len = fd_serializer_build_frame(tx_buf, sizeof(tx_buf),
                                                 CLASS_PUSH,          // The reply class
                                                 TYPE_CONTROL,        // The Type
                                                 CONTROL_SHELL_RLY,  // The Action
                                                 &current_state,      // The data we are sending back
                                                 1);                  // Length of the data (1 byte)
    if (frame_len > 0) 
    {
        fd_protocol_send_dma(tx_buf, frame_len);
        LOG_INF("Pushed Shell Relay state: %d", current_state);
    } 
    else 
    {
        LOG_ERR("Failed to serialize PUSH packet");
    }
}

void pushRefrigDlyRly(void)
{
    uint8_t current_state = 0x01; 
    uint8_t tx_buf[32]; 
    size_t frame_len = fd_serializer_build_frame(tx_buf, sizeof(tx_buf),
                                                 CLASS_PUSH,          // The reply class
                                                 TYPE_CONTROL,        // The Type
                                                 CONTROL_SECOND_STAGE_REFRIG_RLY,  // The Action
                                                 &current_state,      // The data we are sending back
                                                 1);                  // Length of the data (1 byte)
    if (frame_len > 0) 
    {
        fd_protocol_send_dma(tx_buf, frame_len);
        LOG_INF("Pushed Refrigeration Delay / Second Stage Refrig Relay state: %d", current_state);
    } 
    else 
    {
        LOG_ERR("Failed to serialize PUSH packet");
    }
}

void pushVentValve(void)
{
    uint8_t current_state = 0x01; 
    uint8_t tx_buf[32]; 
    size_t frame_len = fd_serializer_build_frame(tx_buf, sizeof(tx_buf),
                                                 CLASS_PUSH,          // The reply class
                                                 TYPE_CONTROL,        // The Type
                                                 CONTROL_VENT_VALVE,  // The Action
                                                 &current_state,      // The data we are sending back
                                                 1);                  // Length of the data (1 byte)
    if (frame_len > 0) 
    {
        fd_protocol_send_dma(tx_buf, frame_len);
        LOG_INF("Pushed Vent Valve state: %d", current_state);
    } 
    else 
    {
        LOG_ERR("Failed to serialize PUSH packet");
    }
}

void pushDefrostRly(void)
{
    uint8_t current_state = 0x01; 
    uint8_t tx_buf[32]; 
    size_t frame_len = fd_serializer_build_frame(tx_buf, sizeof(tx_buf),
                                                 CLASS_PUSH,          // The reply class
                                                 TYPE_CONTROL,        // The Type
                                                 CONTROL_DEFROST_RELAY,  // The Action
                                                 &current_state,      // The data we are sending back
                                                 1);                  // Length of the data (1 byte)
    if (frame_len > 0) 
    {
        fd_protocol_send_dma(tx_buf, frame_len);
        LOG_INF("Pushed Defrost Relay state: %d", current_state);
    } 
    else 
    {
        LOG_ERR("Failed to serialize PUSH packet");
    }
}

void pushMiniChamberHtr(void)
{
    uint8_t current_state = 0x01; 
    uint8_t tx_buf[32]; 
    size_t frame_len = fd_serializer_build_frame(tx_buf, sizeof(tx_buf),
                                                 CLASS_PUSH,          // The reply class
                                                 TYPE_CONTROL,        // The Type
                                                 CONTROL_MINI_CHAMBER_HTR,  // The Action
                                                 &current_state,      // The data we are sending back
                                                 1);                  // Length of the data (1 byte)
    if (frame_len > 0) 
    {
        fd_protocol_send_dma(tx_buf, frame_len);
        LOG_INF("Pushed Mini Chamber state: %d", current_state);
    } 
    else 
    {
        LOG_ERR("Failed to serialize PUSH packet");
    }
}

void pushShelfMstrHtr(void)
{
    uint8_t current_state = 0x01; 
    uint8_t tx_buf[32]; 
    size_t frame_len = fd_serializer_build_frame(tx_buf, sizeof(tx_buf),
                                                 CLASS_PUSH,          // The reply class
                                                 TYPE_CONTROL,        // The Type
                                                 CONTROL_SHELF_MSTR_HTR_CNTRL,  // The Action
                                                 &current_state,      // The data we are sending back
                                                 1);                  // Length of the data (1 byte)
    if (frame_len > 0) 
    {
        fd_protocol_send_dma(tx_buf, frame_len);
        LOG_INF("Pushed Shelf Master Heater state: %d", current_state);
    } 
    else 
    {
        LOG_ERR("Failed to serialize PUSH packet");
    }
}


void pushShelf1Htr(void)
{
    uint8_t current_state = 0x01; 
    uint8_t tx_buf[32]; 
    size_t frame_len = fd_serializer_build_frame(tx_buf, sizeof(tx_buf),
                                                 CLASS_PUSH,          // The reply class
                                                 TYPE_CONTROL,        // The Type
                                                 CONTROL_SHELF1_HTR,  // The Action
                                                 &current_state,      // The data we are sending back
                                                 1);                  // Length of the data (1 byte)
    if (frame_len > 0) 
    {
        fd_protocol_send_dma(tx_buf, frame_len);
        LOG_INF("Pushed Shelf 1 Heater state: %d", current_state);
    } 
    else 
    {
        LOG_ERR("Failed to serialize PUSH packet");
    }
}

void pushShelf2Htr(void)
{
    uint8_t current_state = 0x01; 
    uint8_t tx_buf[32]; 
    size_t frame_len = fd_serializer_build_frame(tx_buf, sizeof(tx_buf),
                                                 CLASS_PUSH,          // The reply class
                                                 TYPE_CONTROL,        // The Type
                                                 CONTROL_SHELF2_HTR,  // The Action
                                                 &current_state,      // The data we are sending back
                                                 1);                  // Length of the data (1 byte)
    if (frame_len > 0) 
    {
        fd_protocol_send_dma(tx_buf, frame_len);
        LOG_INF("Pushed Shelf 2 Heater state: %d", current_state);
    } 
    else 
    {
        LOG_ERR("Failed to serialize PUSH packet");
    }
}


void pushShelf3Htr(void)
{
    uint8_t current_state = 0x01; 
    uint8_t tx_buf[32]; 
    size_t frame_len = fd_serializer_build_frame(tx_buf, sizeof(tx_buf),
                                                 CLASS_PUSH,          // The reply class
                                                 TYPE_CONTROL,        // The Type
                                                 CONTROL_SHELF3_HTR,  // The Action
                                                 &current_state,      // The data we are sending back
                                                 1);                  // Length of the data (1 byte)
    if (frame_len > 0) 
    {
        fd_protocol_send_dma(tx_buf, frame_len);
        LOG_INF("Pushed Shelf 3 Heater state: %d", current_state);
    } 
    else 
    {
        LOG_ERR("Failed to serialize PUSH packet");
    }
}


void pushShelf4Htr(void)
{
    uint8_t current_state = 0x01; 
    uint8_t tx_buf[32]; 
    size_t frame_len = fd_serializer_build_frame(tx_buf, sizeof(tx_buf),
                                                 CLASS_PUSH,          // The reply class
                                                 TYPE_CONTROL,        // The Type
                                                 CONTROL_SHELF4_HTR,  // The Action
                                                 &current_state,      // The data we are sending back
                                                 1);                  // Length of the data (1 byte)
    if (frame_len > 0) 
    {
        fd_protocol_send_dma(tx_buf, frame_len);
        LOG_INF("Pushed Shelf 4 Heater state: %d", current_state);
    } 
    else 
    {
        LOG_ERR("Failed to serialize PUSH packet");
    }
}

void pushShelf5Htr(void)
{
    uint8_t current_state = 0x01; 
    uint8_t tx_buf[32]; 
    size_t frame_len = fd_serializer_build_frame(tx_buf, sizeof(tx_buf),
                                                 CLASS_PUSH,          // The reply class
                                                 TYPE_CONTROL,        // The Type
                                                 CONTROL_SHELF5_HTR,  // The Action
                                                 &current_state,      // The data we are sending back
                                                 1);                  // Length of the data (1 byte)
    if (frame_len > 0) 
    {
        fd_protocol_send_dma(tx_buf, frame_len);
        LOG_INF("Pushed Shelf 5 Heater state: %d", current_state);
    } 
    else 
    {
        LOG_ERR("Failed to serialize PUSH packet");
    }
}

void pushLexsolPumpRly(void)
{
    uint8_t current_state = 0x01; 
    uint8_t tx_buf[32]; 
    size_t frame_len = fd_serializer_build_frame(tx_buf, sizeof(tx_buf),
                                                 CLASS_PUSH,          // The reply class
                                                 TYPE_CONTROL,        // The Type
                                                 CONTROL_LEXSOL_PUMP_RLY,  // The Action
                                                 &current_state,      // The data we are sending back
                                                 1);                  // Length of the data (1 byte)
    if (frame_len > 0) 
    {
        fd_protocol_send_dma(tx_buf, frame_len);
        LOG_INF("Pushed Lexsol Pump Relay state: %d", current_state);
    } 
    else 
    {
        LOG_ERR("Failed to serialize PUSH packet");
    }
}

void pushLexsolHtr(void)
{
    uint8_t current_state = 0x01; 
    uint8_t tx_buf[32]; 
    size_t frame_len = fd_serializer_build_frame(tx_buf, sizeof(tx_buf),
                                                 CLASS_PUSH,          // The reply class
                                                 TYPE_CONTROL,        // The Type
                                                 CONTROL_LEXSOL_HTR,  // The Action
                                                 &current_state,      // The data we are sending back
                                                 1);                  // Length of the data (1 byte)
    if (frame_len > 0) 
    {
        fd_protocol_send_dma(tx_buf, frame_len);
        LOG_INF("Pushed Lexsol state: %d", current_state);
    } 
    else 
    {
        LOG_ERR("Failed to serialize PUSH packet");
    }
}

void pushRefValveRly(void)
{
    uint8_t current_state = 0x01; 
    uint8_t tx_buf[32]; 
    size_t frame_len = fd_serializer_build_frame(tx_buf, sizeof(tx_buf),
                                                 CLASS_PUSH,          // The reply class
                                                 TYPE_CONTROL,        // The Type
                                                 CONTROL_REF_VALVE_RLY,  // The Action
                                                 &current_state,      // The data we are sending back
                                                 1);                  // Length of the data (1 byte)
    if (frame_len > 0) 
    {
        fd_protocol_send_dma(tx_buf, frame_len);
        LOG_INF("Pushed Reference Valve state: %d", current_state);
    } 
    else 
    {
        LOG_ERR("Failed to serialize PUSH packet");
    }
}

void pushTiltValve(void)
{
    uint8_t current_state = 0x01; 
    uint8_t tx_buf[32]; 
    size_t frame_len = fd_serializer_build_frame(tx_buf, sizeof(tx_buf),
                                                 CLASS_PUSH,          // The reply class
                                                 TYPE_CONTROL,        // The Type
                                                 CONTROL_TILT_VALVE,  // The Action
                                                 &current_state,      // The data we are sending back
                                                 1);                  // Length of the data (1 byte)
    if (frame_len > 0) 
    {
        fd_protocol_send_dma(tx_buf, frame_len);
        LOG_INF("Pushed Tilt Valve state: %d", current_state);
    } 
    else 
    {
        LOG_ERR("Failed to serialize PUSH packet");
    }
}

void push5VFanDrv(void)
{
    uint8_t current_state = 0x01; 
    uint8_t tx_buf[32]; 
    size_t frame_len = fd_serializer_build_frame(tx_buf, sizeof(tx_buf),
                                                 CLASS_PUSH,          // The reply class
                                                 TYPE_CONTROL,        // The Type
                                                 CONTROL_5V_FAN_DRV,  // The Action
                                                 &current_state,      // The data we are sending back
                                                 1);                  // Length of the data (1 byte)
    if (frame_len > 0) 
    {
        fd_protocol_send_dma(tx_buf, frame_len);
        LOG_INF("Pushed 5v fan Driver state: %d", current_state);
    } 
    else 
    {
        LOG_ERR("Failed to serialize PUSH packet");
    }
}
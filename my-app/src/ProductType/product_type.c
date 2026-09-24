/*
This files contains logic to determine the product and accessory type
Step 1. Read product type from EEPROM.
Step 2. If it is unprogrammed board, load Product ID = 0x3F(as per FD 2.0)
Step 3. If programmed board, collect product type from EEPROM.
Step 4. If model number = 123456789, board is in CM Test Phase. Utilize 
        GPIO pin state to determine the Product Type.
Step 5. This main firmware could be either FD/Triad. If Tilt Valve present 
        implies it is a Triad. else FD.
Step 5. Accessory information can be retrieved from the Message from 2dary IOB.
*/

#include "product_type.h"
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

uint8_t FindProductType(void);
uint8_t FindAccessoryType(void);

extern void ReadAccessoryType(void);
bool Tilt_Valve_Present()
{
    // Read Valve present here.
    return true;
}

bool Vent_Valve_Present() 
{
    // Read Vent Valve here
    return true;
}
uint8_t FindProductType(void)
{
    // Product type could be either FD or Triad
    //Triad has only Tilt SW sensor, Shelf Tilt Valve, Ref Valve
    
    if (Tilt_Valve_Present() == true)
    {
        return IAM_TRIAD;
    }
    // If it is not a TRIAD, then by default it is a FD.
    // Else loop here gives deterministic characterisric on presence of Vent Valve
    else if ((Tilt_Valve_Present() != true) && (Vent_Valve_Present() == true))
    {
        return IAM_FD;
    }
    
    else
    {
        return IAM_UNKNOWN;
        // I am unknown implies either Title Valve or Vent Valve is not connected.
    }
    return IAM_UNKNOWN ;
}

uint8_t FindAccessoryType(void)
{
    // Implement logic to determine the Accessory ID here
    // Accessory information can be retrieved from the 2ndary IOB via uart.
    // Let us make it request-response format.
    ReadAccessoryType();

    return IAM_TRIAD;
}
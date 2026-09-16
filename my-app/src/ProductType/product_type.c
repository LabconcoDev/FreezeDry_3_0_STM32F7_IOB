/*
This files contains logic to determine the product and accessory type

*/

#include "product_type.h"
#include <stdint.h>

uint8_t FindProductType(void);
uint8_t FindAccessoryType(void);
uint8_t FindProductType(void)
{
    return IAM_FD;
}

uint8_t FindAccessoryType(void)
{
    // Implement logic to determine the product ID here
    return IAM_TRIAD;
}
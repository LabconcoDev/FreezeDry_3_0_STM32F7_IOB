/* 
1. Triad might be part of product type, might need to update accordingly
2. #define might be used instead. 
*/

#ifndef PRODUCT_TYPE_H
#define PRODUCT_TYPE_H


#include <stddef.h>
#include <stdint.h>

uint8_t FindProductType(void);
uint8_t FindAccessoryType(void);

typedef enum
{
    IAM_UNKNOWN = 0x0E,

    IAM_FD      = 0x01,
    IAM_TRIAD   = 0x02,
    IAM_BT      = 0x03,
    IAM_ST      = 0x04

} product_id_t;


#endif /* PRODUCT_TYPE_H */
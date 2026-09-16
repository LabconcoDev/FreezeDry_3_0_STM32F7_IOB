#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "task_manager.h"
#include "producttype/product_type.h"
LOG_MODULE_REGISTER(main_app, LOG_LEVEL_INF);


uint8_t u8_ProductID;
uint8_t u8_AccessoryID;

int main(void)
{
    LOG_INF("System boot complete");
    start_all_tasks();


    // Function to determine the product type to be called here. This is based on Tilt Valve and Vent valve?
    // FindProductType();
    // FindAccessoryType();
    // For development purpose let us hardcode it here.
    // This controller will always be FD, but accessory might be different. The communication to be handled based on accessory attached.
    // Hence accessory ID to be determined correctly.

    u8_ProductID     = FindProductType(); 
    u8_AccessoryID   = FindAccessoryType(); 

    while (1) {
        LOG_INF("Main ENTERED");
        k_yield();
        k_msleep(5000);
    }
}
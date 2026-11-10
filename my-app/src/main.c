#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "boot_sequence.h"
#include "task_manager.h"

LOG_MODULE_REGISTER(main_app, LOG_LEVEL_INF);

int main(void)
{
    system_boot_sequence();

    while (1) 
    {
        k_msleep(5000);
        reset_timeline_trace();
        LOG_INF("Main ENTERED");
    }

    return 0;
}
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "task_manager.h"

LOG_MODULE_REGISTER(main_app, LOG_LEVEL_INF);

int main(void)
{
    LOG_INF("System boot complete");
    start_all_tasks();

    while (1) {
        LOG_INF("Main ENTERED");
        k_yield();
        k_msleep(5000);
    }
}
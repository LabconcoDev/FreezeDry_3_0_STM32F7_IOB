#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(stack_test);

void print_stack_usage(void)
{
    size_t unused;

    int ret = k_thread_stack_space_get(k_current_get(), &unused);

    if (ret == 0) 
    {
    //    //LOG_INF("Thread %s unused stack: %u bytes",
    //             k_thread_name_get(k_current_get()),
    //             unused);
    }
}
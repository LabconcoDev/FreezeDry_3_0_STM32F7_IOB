#ifndef TASK_CONTROL_H
#define TASK_CONTROL_H

#include <stddef.h>
#include <stdint.h>

int  control_task_init(void);
void control_task_run(void);
int  control_send_cmd(uint8_t action, const uint8_t *data, size_t data_len);

#endif /* TASK_CONTROL_H */
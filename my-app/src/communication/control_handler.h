#ifndef CONTROL_HANDLER_H
#define CONTROL_HANDLER_H

#include "protocol_defs.h"
#include <stddef.h>
#include <stdint.h>

void fd_control_handler_process(uint8_t cls, uint8_t action, const uint8_t *data, size_t data_len);
void ControlHardware_Handler(uint8_t action, const uint8_t *data);
#endif /* CONTROL_HANDLER_H */
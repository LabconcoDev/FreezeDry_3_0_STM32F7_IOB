#ifndef SETTINGS_HANDLER_H
#define SETTINGS_HANDLER_H

#include "parser.h"
#include "protocol_defs.h"
#include <stddef.h>
#include <stdint.h>

void fd_settings_handler_process(fd_interface_t source_if, uint8_t cls, uint8_t action, const uint8_t *data, size_t data_len);

#endif /* SETTINGS_HANDLER_H */
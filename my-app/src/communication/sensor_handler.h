#ifndef SENSOR_HANDLER_H
#define SENSOR_HANDLER_H

#include <zephyr/logging/log.h>
#include "serializer.h"
#include "product_type.h"
#include "protocol_defs.h"
#include <stddef.h>
#include <stdint.h>

void fd_sensor_handler_process(uint8_t cls, uint8_t action, const uint8_t *data, size_t data_len);
void PushSensorData(uint8_t action);
#endif /* SENSOR_HANDLER_H */
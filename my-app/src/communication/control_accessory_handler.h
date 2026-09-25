/*
 * control_accessory_handler.h
 *
 * Handles the validation, serialization, and UART forwarding of control 
 * commands that belong to secondary accessories (e.g., ST, BT).
 */

#ifndef CONTROL_ACCESSORY_HANDLER_H_
#define CONTROL_ACCESSORY_HANDLER_H_

#include <stdint.h>
#include <stddef.h>

/**
 * @brief Validates if an accessory is present and forwards the control command via UART.
 * 
 * @param cls       The protocol Class (e.g., CLASS_WRITE or CLASS_READ)
 * @param action    The protocol Action byte 
 * @param data      Pointer to the payload data
 * @param data_len  Length of the payload data
 */
void accessory_control_handler_process(uint8_t action, const uint8_t *data, size_t data_len);

#endif /* CONTROL_ACCESSORY_HANDLER_H_ */
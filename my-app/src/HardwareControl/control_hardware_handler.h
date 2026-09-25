/*
 * control_hardware_handler.h
 *
 * Handles all physical GPIO manipulation for the Main Base Unit (FD) and Triad.

 */

#ifndef CONTROL_HARDWARE_HANDLER_H_
#define CONTROL_HARDWARE_HANDLER_H_

#include <stdint.h>
#include <stddef.h>

/**
 * @brief Initializes all local GPIO pins for control relays/valves.
 *        Should be called once during system boot in main.c or task_manager.
 */
void fd_control_hardware_init(void);

/**
 * @brief Executes a physical hardware change based on the requested action.
 * 
 * @param action    The protocol ACTION byte (e.g., CONTROL_VACUUM_RLY)
 * @param data      Pointer to the payload data (e.g., On/Off state)
 * @param data_len  Length of the payload data
 */
void fd_control_hardware_execute(uint8_t action, const uint8_t *data);

#endif /* CONTROL_HARDWARE_HANDLER_H_ */
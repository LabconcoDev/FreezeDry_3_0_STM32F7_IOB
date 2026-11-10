#ifndef BOOT_SEQUENCE_H
#define BOOT_SEQUENCE_H

#include <stdint.h>

/* Global Product & Accessory IDs */
extern uint8_t u8_ProductID;
extern uint8_t u8_AccessoryID;

/* Stage 1: Start Comms Threads, Discover Product/Accessory, Init Buffers, Send IAM_ALIVE */
void boot_stage1_comms_and_discovery(void);

/* Stage 2: Start Periodic Real-Time Tasks (Control, Sensor, Alert, Telemetry) */
void boot_stage2_periodic_tasks(void);

/* Master Boot Function called from main() */
void system_boot_sequence(void);

#endif /* BOOT_SEQUENCE_H */
#ifndef TASK_SENSOR_H
#define TASK_SENSOR_H

int  sensor_task_init(void);
void sensor_task_run(void);   /* Runs every 50 ms in 4_Sensor_Thread */

#endif /* TASK_SENSOR_H */
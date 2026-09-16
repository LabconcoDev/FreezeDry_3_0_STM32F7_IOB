#ifndef TASK_ADS_H_
#define TASK_ADS_H_

/**
 * @brief Initialize the ADS124S08 peripheral and driver.
 * @return 0 on success, negative error code on failure.
 */
int ads_task_init(void);

/**
 * @brief Main handler for the ADS124S08 task.
 * Reads ADC values and updates the global state.
 */
void ads_task_run(void);

/**
 * @brief Thread-safe getter to retrieve the latest ADC/RTD value.
 * @return The current 24-bit processed value.
 */
float get_ads_rtd_value(void);

#endif /* TASK_ADS_H_H */
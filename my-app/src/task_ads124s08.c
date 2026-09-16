#include <zephyr/kernel.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>
#include "task_ads124s08.h"
#include <math.h>
LOG_MODULE_REGISTER(task_ads, LOG_LEVEL_INF);



/* RTD constants */
#define VREF            2.5f
#define IDAC_CURRENT    0.0005f
#define ADC_FS          8388608.0f

#define RREF        2000.0f
#define ADC_FS      8388608.0f

/* PT1000 Callendar-Van Dusen coefficients */
#define RTD_R0          1000.0f
#define RTD_A           3.9083e-3f
#define RTD_B          -5.775e-7f




const struct device *ads_dev = DEVICE_DT_GET_ANY(ti_ads124s08);
/*
 * Channel configuration
 */
static struct adc_channel_cfg ads_channel_cfg =
{

    .channel_id = 0,
    .differential = true,
    .input_positive = 1,     // AIN0
    .input_negative = 0,     // AIN0
    .gain = ADC_GAIN_1,
    .reference = ADC_REF_EXTERNAL1,
    .acquisition_time = ADC_ACQ_TIME_DEFAULT,
    .current_source_pin_set = true,
    .current_source_pin =
    {
        5,      // I1MUX -> AIN5
        15      // I2MUX -> OFF
    }
};


/*
 * Convert ADC code to RTD resistance
 */
static float adc_to_resistance(int32_t adc)
{
     return ((float)adc / ADC_FS) * RREF;
}



/*
 * Convert PT1000 resistance to temperature
 *
 * Valid for temperature > 0C
 */
static float resistance_to_temperature(float resistance)
{
    float temp;


    temp = (-RTD_A +    sqrtf(     RTD_A * RTD_A -    4 * RTD_B *     (1 - resistance / RTD_R0)  ))   /      (2 * RTD_B);

    return temp;
}

int ads_task_init(void) {
    if (!device_is_ready(ads_dev)) {
        LOG_ERR("ADS124S08 not ready");
        return -ENODEV;
    }
    int ret = adc_channel_setup(
            ads_dev,
            &ads_channel_cfg);


    if(ret)
    {
        LOG_ERR("ADC channel setup failed %d", ret);
        return ret;
    }


    LOG_INF("ADS124S08 channel configured");
    return 0;
}

void ads_task_run(void)
{
    int err;
    int32_t adc_value;

    struct adc_sequence sequence = {
        .channels = BIT(0),
        .buffer = &adc_value,
        .buffer_size = sizeof(adc_value),
        .resolution = 24,
        .oversampling = 0,
        .calibrate = false,
    };

    err = adc_read(ads_dev, &sequence);

    if (err == 0) {
        printk("ADC raw = %d\n", adc_value);
    }
    else {
        printk("ADC read failed %d\n", err);
    }

    float resistance;
    float temperature;


    resistance =  adc_to_resistance(  adc_value);
    temperature =   resistance_to_temperature(   resistance);
    LOG_INF(     "ADC = %d",        adc_value);
    LOG_INF(     "RTD = %.2f ohm",     (double)resistance);
    LOG_INF(    "Temp = %.2f C",     (double)temperature);
}
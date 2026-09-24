#include "system_config.h"
#include <zephyr/kernel.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>
#include "task_ads124s08.h"
#include <math.h>

#include "DataStorage/main_iob_data.h"
#include "communication/protocol_defs.h"

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


/* Helper function to chop an int16_t into 2 bytes and send it to memory */
static void save_temp_to_memory(uint8_t action_id, int16_t temp_value)
{
    uint8_t raw_data[2];
    
    // Convert 16-bit integer into a 2-byte array (Little Endian)
    raw_data[0] = temp_value & 0xFF;         // LSB
    raw_data[1] = (temp_value >> 8) & 0xFF;  // MSB
    
    UpdateMainIOBData(action_id, raw_data);
}



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



int ads_task_init(void) 
{
#ifndef SIMULATION
    if (!device_is_ready(ads_dev)) 
    {
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
#else
    LOG_INF("ADS124S08 initialized in SIMULATION mode (bypassing hardware checks)");
#endif

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



#ifdef SIMULATION
    /* --- SIMULATION CODE --- */
    static int16_t fake_collector = 200;  // 20.0 °C
    static int16_t fake_shelf     = 100;  // 10.0 °C
    
    fake_collector += 5;                 
    fake_shelf     += 2;     
    if (fake_collector > 800) fake_collector = -200;           
    if (fake_shelf > 400) fake_shelf = 0;
    // Safely inject our fake data
    save_temp_to_memory(SENSOR_COLLECTOR_TEMP, fake_collector);
    save_temp_to_memory(SENSOR_SHELL_DRY_LEXSOL, fake_collector - 50);
    save_temp_to_memory(SENSOR_SHELF1_TEMP, fake_shelf);
    save_temp_to_memory(SENSOR_SAMPLE1_TEMP, fake_shelf + 10);

#else

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

#endif

}
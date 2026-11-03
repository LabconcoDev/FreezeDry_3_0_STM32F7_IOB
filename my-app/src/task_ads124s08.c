#include "build_config.h"
#include <zephyr/kernel.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>
#include "task_ads124s08.h"
#include <math.h>
#include <zephyr/drivers/gpio.h>
#include "DataStorage/main_iob_data.h"
#include "communication/protocol_defs.h"

LOG_MODULE_REGISTER(task_ads, LOG_LEVEL_INF);

/* RTD constants */
#define VREF            2.5f
#define IDAC_CURRENT    0.00025f
#define ADC_FS          8388608.0f

#define RREF        2058.0f


/* PT1000 Callendar-Van Dusen coefficients */
#define RTD_R0          1000.0f
#define RTD_A           3.9083e-3f
#define RTD_B          -5.775e-7f

#define ADS_AINCOM       12  /* Zephyr internal index for AINCOM */

#define MUX_NODE DT_PATH(rtd_mux)

static const struct gpio_dt_spec mux_a0 = GPIO_DT_SPEC_GET(DT_ALIAS(mux_a0), gpios);//PC4
static const struct gpio_dt_spec mux_a1 = GPIO_DT_SPEC_GET(DT_ALIAS(mux_a1), gpios);//PE9
static const struct gpio_dt_spec mux_a2 = GPIO_DT_SPEC_GET(DT_ALIAS(mux_a2), gpios);//PC9
static const struct gpio_dt_spec mux_en1 = GPIO_DT_SPEC_GET(DT_ALIAS(mux_en1), gpios);//PB9
static const struct gpio_dt_spec mux_en2 = GPIO_DT_SPEC_GET(DT_ALIAS(mux_en2), gpios);//PA1

/* Helper to drive address lines 000 to 111 */
static void set_rtd_mux_channel(uint8_t address)
{
    /* Address bits 0-2 */
    gpio_pin_set_dt(&mux_a0, (address >> 0) & 0x01); // PC4 (bit 0)
    gpio_pin_set_dt(&mux_a1, (address >> 1) & 0x01); // PE9 (bit 1)
    gpio_pin_set_dt(&mux_a2, (address >> 2) & 0x01); // PC9 (bit 2)

    /* 2. MUX Enable (Bit 3):
     * address 0-7  (bit 3 = 0) -> Select MUX 1: PB9 = 1, PA1 = 0
     * address 8-10 (bit 3 = 1) -> Select MUX 2: PB9 = 0, PA1 = 1
     */
    bool select_mux2 = (address >> 3) & 0x01;
    if (!select_mux2) 
    {
        gpio_pin_set_dt(&mux_en1, 1);
        gpio_pin_set_dt(&mux_en2, 0); // PA1 is NOT(PB9)
    } 
    else 
    {
        gpio_pin_set_dt(&mux_en1, 0);
        gpio_pin_set_dt(&mux_en2, 1); // PA1 is NOT(PB9)
    }
}

/* ========================================================
 * PRODUCT-SPECIFIC RTD CONFIGURATIONS
 * ======================================================== */
struct rtd_config {
    uint8_t channel_id;
    uint8_t input_positive; // ADS124S08 sense pin (AIN3 to AIN10)
    uint8_t input_negative; // ADS_AINCOM (12)
    uint8_t idac_pin;        // ALWAYS 11 (Excitation routes out AIN11)
    uint8_t mux_channel;    // 0 to 7 (Address for PC9, PE9, PC4)
    uint8_t action_id;
};
/* 1a. Freeze Dry (With Mini Chamber) */
static const struct rtd_config fd_mini_chamber_configs[] = {
    { .channel_id = 0, .input_positive = 1, .input_negative = 0, .idac_pin = 5, .action_id = SENSOR_COLLECTOR_TEMP },
    { .channel_id = 1, .input_positive = 3, .input_negative = 2, .idac_pin = 5, .action_id = SENSOR_MINI_CHAMBER_TEMP }
};
/* 1b. Freeze Dry (With Shell Freezer) */
static const struct rtd_config fd_shell_configs[] = {
    { .channel_id = 0, .input_positive = 1, .input_negative = 0, .idac_pin = 5, .action_id = SENSOR_COLLECTOR_TEMP },
    { .channel_id = 1, .input_positive = 3, .input_negative = 2, .idac_pin = 5, .action_id = SENSOR_SHELL_FREEZER_TEMP_SENSOR }
};

/* 2. Triad (4 RTDs) */
static const struct rtd_config triad_configs[] = 
{
    { .channel_id = 0, .input_positive = 1, .input_negative = 0, .idac_pin = 5, .action_id = SENSOR_COLLECTOR_TEMP},
    { .channel_id = 1, .input_positive = 3, .input_negative = 2, .idac_pin = 5, .action_id = SENSOR_SHELF1_TEMP }, 
    { .channel_id = 2, .input_positive = 4, .input_negative = 6, .idac_pin = 5, .action_id = SENSOR_SAMPLE1_TEMP },
    { .channel_id = 3, .input_positive = 7, .input_negative = 8, .idac_pin = 5, .action_id = SENSOR_LEXSOL_TEMP}
};

/* 3. Max Config (11 RTDs) */

static const struct rtd_config max_configs[] = 
{
    /* MUX 000 -> Sensor 1 on AIN3 */
    { .channel_id = 0, .input_positive = 3,  .input_negative = ADS_AINCOM, .idac_pin = 11, .mux_channel = 0, .action_id = SENSOR_RTD_TEMP1 },
    /* MUX 001 -> Sensor 2 on AIN4 */
    { .channel_id = 0, .input_positive = 4,  .input_negative = ADS_AINCOM, .idac_pin = 11, .mux_channel = 1, .action_id = SENSOR_RTD_TEMP2 },
    /* MUX 010 -> Sensor 3 on AIN5 */
    { .channel_id = 0, .input_positive = 5,  .input_negative = ADS_AINCOM, .idac_pin = 11, .mux_channel = 2, .action_id = SENSOR_RTD_TEMP3 },
    /* MUX 011 -> Sensor 4 on AIN6 */
    { .channel_id = 0, .input_positive = 6,  .input_negative = ADS_AINCOM, .idac_pin = 11, .mux_channel = 3, .action_id = SENSOR_RTD_TEMP4 },
    /* MUX 100 -> Sensor 5 on AIN7 */
    { .channel_id = 0, .input_positive = 7,  .input_negative = ADS_AINCOM, .idac_pin = 11, .mux_channel = 4, .action_id = SENSOR_RTD_TEMP5 },
    /* MUX 101 -> Sensor 6 on AIN8 */
    { .channel_id = 0, .input_positive = 8,  .input_negative = ADS_AINCOM, .idac_pin = 11, .mux_channel = 5, .action_id = SENSOR_RTD_TEMP6 },
    /* MUX 110 -> Sensor 7 on AIN9 */
    { .channel_id = 0, .input_positive = 9,  .input_negative = ADS_AINCOM, .idac_pin = 11, .mux_channel = 6, .action_id = SENSOR_RTD_TEMP7 },
    /* MUX 111 -> Sensor 8 on AIN10 */
    { .channel_id = 0, .input_positive = 10, .input_negative = ADS_AINCOM, .idac_pin = 11, .mux_channel = 7, .action_id = SENSOR_RTD_TEMP8 },

    /* --- MUX 2 (AIN0, AIN1, AIN2) --- */
    /* MUX 000 on MUX2 -> Sensor 9 on AIN0 */
    { .channel_id = 0, .input_positive = 0,  .input_negative = ADS_AINCOM, .idac_pin = 11, .mux_channel = 8,  .action_id = SENSOR_RTD_TEMP9 },
    /* MUX 001 on MUX2 -> Sensor 10 on AIN1 */
    { .channel_id = 0, .input_positive = 1,  .input_negative = ADS_AINCOM, .idac_pin = 11, .mux_channel = 9,  .action_id = SENSOR_RTD_TEMP10 },
    /* MUX 010 on MUX2 -> Sensor 11 on AIN2 */
    { .channel_id = 0, .input_positive = 2,  .input_negative = ADS_AINCOM, .idac_pin = 11, .mux_channel = 10, .action_id = SENSOR_RTD_TEMP11 },
};


/* --- DYNAMIC POINTERS --- */
static const struct rtd_config *active_rtds = NULL;
static uint8_t num_active_rtds = 0;




const struct device *ads_dev = DEVICE_DT_GET_ANY(ti_ads124s08);


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
    
    /* to be configured based on product type */
    active_rtds = max_configs;
    num_active_rtds = ARRAY_SIZE(max_configs);
    
    /* Configure PC9, PE9, PC4 as outputs set to 0 */
    gpio_pin_configure_dt(&mux_a0, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&mux_a1, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&mux_a2, GPIO_OUTPUT_INACTIVE);

    /* Configure PB9 and PA1 as outputs (initially PB9=0, PA1=1) */
    gpio_pin_configure_dt(&mux_en1, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&mux_en2, GPIO_OUTPUT_ACTIVE);



#ifndef SIMULATION
    if (!device_is_ready(ads_dev)) 
    {
        LOG_ERR("ADS124S08 not ready");
        return -ENODEV;
    }
    LOG_INF("ADS124S08 device ready");
#endif
    return 0;


}

void ads_task_run(void)
{

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
    /* --- HARDWARE CODE --- */
    int err;
    int32_t adc_val1 = 0;
    int32_t adc_val2 = 0;
    /* Read all 11 RTDs sequentially */

    
    for (int i = 0; i < num_active_rtds; i++) 
    {
        k_msleep(200);
        /* 1. Select the sensor via the external MUX */
        set_rtd_mux_channel(active_rtds[i].mux_channel);
        /* 2. Settle time for MUX switch and analog filter (e.g. 1 ms) */
        k_msleep(200);
        /* 3. Setup ADS124S08: IDAC1 stays on AIN11, sense on active input_positive */
        struct adc_channel_cfg cfg = {
            .channel_id = 0,
            .differential = true,
            .input_positive = active_rtds[i].input_positive,
            .input_negative = active_rtds[i].input_negative,
            .gain = ADC_GAIN_1,
            .reference = ADC_REF_EXTERNAL0,
            .acquisition_time = ADC_ACQ_TIME_DEFAULT,
            .current_source_pin_set = true,
            .current_source_pin = { 11, 15 } /* IDAC1 on AIN11, IDAC2 off */
        };
             
        
        
        err = adc_channel_setup(ads_dev, &cfg);
        if (err != 0) 
        {
            LOG_ERR("ADC MUX setup failed for RTD%d: %d", i + 1, err);
            continue;
        }
        
        /* Declare the sequence buffer for reading channel 0 */
        struct adc_sequence sequence = {
            .channels = BIT(0),
            .buffer = &adc_val1,
            .buffer_size = sizeof(adc_val1),
            .resolution = 24,
            .oversampling = 0,
            .calibrate = false,
        };
        k_msleep(500);
        
        /* 1st Read */
        sequence.buffer = &adc_val1;
        adc_read(ads_dev, &sequence);
        float r1 = adc_to_resistance(adc_val1);
        float t1 = resistance_to_temperature(r1);
        /* 2nd Read */
        sequence.buffer = &adc_val2;
        err = adc_read(ads_dev, &sequence);
        if (err == 0) 
        {
            float r2 = adc_to_resistance(adc_val2);
            float t2 = resistance_to_temperature(r2);
            save_temp_to_memory(active_rtds[i].action_id, (int16_t)(t2 * 10.0f));
            /* Print 1st Read then 2nd Read */
                        /* Print 1st Read then 2nd Read with Code in Decimal */
            printk("\x1b[1;0uRTD%02d (AIN%d) [Read 1]: Code=%ld | R=%.2f Ohm | Temp=%.2f C\n",
                i + 1, active_rtds[i].input_positive,
                (long)adc_val1, (double)r1, (double)t1);

            printk("RTD%02d (AIN%d) [Read 2]: Code=%ld | R=%.2f Ohm | Temp=%.2f C\n\x1b[0;0u",
                i + 1, active_rtds[i].input_positive,
                (long)adc_val2, (double)r2, (double)t2);
        }
        
        // /* 1st Read: Dummy read to flush digital filter and discard */
        // adc_read(ads_dev, &sequence);
        // /* 2nd Read: Valid, fully settled reading to keep */
        // err = adc_read(ads_dev, &sequence);
        // if (err == 0) 
        // {
        //     float resistance = adc_to_resistance(adc_value);
        //     float temperature = resistance_to_temperature(resistance);
            
        //     /* Scale float (20.5 C) to protocol int16_t (205) */
        //     int16_t temp_scaled = (int16_t)(temperature * 10.0f);
            
        //     /* Save to memory using its SENSOR_RTD_TEMP ID */
        //     save_temp_to_memory(active_rtds[i].action_id, temp_scaled);
            
        //     // printk("RTD%02d (AIN%d): Code=0x%06X | R=%.2f Ohm | Temp=%.2f C\n",  i + 1, active_rtds[i].input_positive,(uint32_t)(adc_value & 0x00FFFFFF),
        //     //         (double)resistance, (double)temperature);

        //     /* \x1b[1;0u switches to Terminal 1, \x1b[0;0u switches back to Terminal 0 */
        //     printk("\x1b[1;0uRTD%02d (AIN%d): Code=0x%06X | R=%.2f Ohm | Temp=%.2f C\n\x1b[0;0u",
        //         i + 1, active_rtds[i].input_positive,
        //         (uint32_t)(adc_value & 0x00FFFFFF),
        //         (double)resistance, (double)temperature);
        // } 
        // else 
        // {
        //     LOG_ERR("ADC read failed on RTD%d: %d", i + 1, err);
        // }
    }
#endif
}
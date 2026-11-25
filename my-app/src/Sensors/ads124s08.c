#include "build_config.h"
#include <zephyr/kernel.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>
#include <math.h>
#include "ads124s08.h"
#include "product_type.h"
#include "communication/protocol_defs.h"
#include "zbus_channels.h"

LOG_MODULE_REGISTER(ads124s08, LOG_LEVEL_INF);

extern uint8_t u8_ProductID;

/* RTD constants */
#define VREF            2.5f
#define IDAC_CURRENT    0.000500f
#define ADC_FS          8388608.0f
#define RREF            2058.0f

/* PT1000 Callendar-Van Dusen coefficients */
#define RTD_R0          1000.0f
#define RTD_A           3.9083e-3f
#define RTD_B          -5.775e-7f

#define ADS_AINCOM      12  /* Zephyr internal index for AINCOM */

static const struct gpio_dt_spec mux_a0  = GPIO_DT_SPEC_GET(DT_ALIAS(mux_a0), gpios);  /* PC4 */
static const struct gpio_dt_spec mux_a1  = GPIO_DT_SPEC_GET(DT_ALIAS(mux_a1), gpios);  /* PE9 */
static const struct gpio_dt_spec mux_a2  = GPIO_DT_SPEC_GET(DT_ALIAS(mux_a2), gpios);  /* PC9 */
static const struct gpio_dt_spec mux_en1 = GPIO_DT_SPEC_GET(DT_ALIAS(mux_en1), gpios); /* PB9 */
static const struct gpio_dt_spec mux_en2 = GPIO_DT_SPEC_GET(DT_ALIAS(mux_en2), gpios); /* PA1 */

static void init_sys_temp_msg(struct sys_temp_msg *msg)
{
    msg->collector_dC         = 0x7FFF;
    msg->ambient_dC           = 0x7FFF;
    msg->hx_1st_stage_dC      = 0x7FFF;
    msg->hx_2nd_stage_dC      = 0x7FFF;
    msg->suction_1st_stage_dC = 0x7FFF;
    msg->suction_2nd_stage_dC = 0x7FFF;
    msg->shell_or_mini_dC     = 0x7FFF;
    msg->triad_lexsol_dC      = 0x7FFF;
    msg->triad_shelf_dC       = 0x7FFF;
    msg->triad_sample_dC      = 0x7FFF;
    msg->timestamp_ms         = k_uptime_get_32();
    msg->valid                = true;
}

#ifdef SIMULATION
static void simulate_temperatures(struct sys_temp_msg *msg)
{
    typedef struct
    {
        int16_t val;
        int16_t min_dC;
        int16_t max_dC;
        int8_t  dir;
        uint8_t divider; /* ticks per 0.1 C step */
    } sim_temp_t;

    static sim_temp_t sim[10] =
    {
        /* 0: Collector      */ { -450, -550,  250, -1, 10 },
        /* 1: Ambient        */ {  220,  180,  280,  1, 80 },
        /* 2: 1st HX Outlet  */ { -350, -450,  200, -1, 15 },
        /* 3: 2nd HX Outlet  */ { -600, -850,  200, -1, 15 },
        /* 4: 1st Suction    */ { -150, -300,  150,  1, 20 },
        /* 5: 2nd Suction    */ { -250, -450,  150,  1, 20 },
        /* 6: Shell / Mini   */ {  200, -400,  350, -1, 15 },
        /* 7: Triad Lexsol   */ {  150, -550,  350,  1, 25 },
        /* 8: Triad Shelf    */ { -200, -550,  400,  1, 20 },
        /* 9: Triad Sample   */ { -180, -550,  400,  1, 30 },
    };
    static uint32_t sim_tick = 0;

    sim_tick++;
    for (int i = 0; i < 10; i++)
    {
        if ((sim_tick % sim[i].divider) == 0U)
        {
            sim[i].val += sim[i].dir;
            if (sim[i].val >= sim[i].max_dC || sim[i].val <= sim[i].min_dC)
            {
                sim[i].dir = -sim[i].dir;
            }
        }
    }

    msg->collector_dC         = sim[0].val;
    msg->ambient_dC           = sim[1].val;
    msg->hx_1st_stage_dC      = sim[2].val;
    msg->hx_2nd_stage_dC      = sim[3].val;
    msg->suction_1st_stage_dC = sim[4].val;
    msg->suction_2nd_stage_dC = sim[5].val;

    if (u8_ProductID == IAM_FD)
    {
        msg->shell_or_mini_dC = sim[6].val;
    }
    else if (u8_ProductID == IAM_TRIAD)
    {
        msg->triad_lexsol_dC  = sim[7].val;
        msg->triad_shelf_dC   = sim[8].val;
        msg->triad_sample_dC  = sim[9].val;
    }
}
#else
struct rtd_config
{
    uint8_t channel_id;
    uint8_t input_positive; /* ADS124S08 sense pin (AIN3 to AIN10) */
    uint8_t input_negative; /* ADS_AINCOM (12) */
    uint8_t idac_pin;       /* ALWAYS 11 (Excitation routes out AIN11) */
    uint8_t mux_channel;    /* 0 to 10 (Address for PC9, PE9, PC4) */
    uint8_t action_id;
};

static const struct rtd_config max_configs[] =
{
    { .channel_id = 0, .input_positive = 3,  .input_negative = ADS_AINCOM, .idac_pin = 11, .mux_channel = 0,  .action_id = SENSOR_COLLECTOR_TEMP },
    { .channel_id = 0, .input_positive = 4,  .input_negative = ADS_AINCOM, .idac_pin = 11, .mux_channel = 1,  .action_id = SENSOR_AMBIENT_TEMPERATURE },
    { .channel_id = 0, .input_positive = 5,  .input_negative = ADS_AINCOM, .idac_pin = 11, .mux_channel = 2,  .action_id = SENSOR_LEXSOL_TEMP },
    { .channel_id = 0, .input_positive = 6,  .input_negative = ADS_AINCOM, .idac_pin = 11, .mux_channel = 3,  .action_id = SENSOR_FIRST_STAGE_HEAT_EXCHANGER_OUTLET_TEMP },
    { .channel_id = 0, .input_positive = 7,  .input_negative = ADS_AINCOM, .idac_pin = 11, .mux_channel = 4,  .action_id = SENSOR_SECOND_STAGE_HEAT_EXCHANGER_OUTLET_TEMP },
    { .channel_id = 0, .input_positive = 8,  .input_negative = ADS_AINCOM, .idac_pin = 11, .mux_channel = 5,  .action_id = SENSOR_FIRST_STAGE_SUCTION_TEMP },
    { .channel_id = 0, .input_positive = 9,  .input_negative = ADS_AINCOM, .idac_pin = 11, .mux_channel = 6,  .action_id = SENSOR_SECOND_STAGE_SUCTION_TEMP },
    { .channel_id = 0, .input_positive = 10, .input_negative = ADS_AINCOM, .idac_pin = 11, .mux_channel = 7,  .action_id = SENSOR_SHELL_FREEZER_TEMP_SENSOR },
    { .channel_id = 0, .input_positive = 0,  .input_negative = ADS_AINCOM, .idac_pin = 11, .mux_channel = 8,  .action_id = SENSOR_MINI_CHAMBER_TEMP },
    { .channel_id = 0, .input_positive = 1,  .input_negative = ADS_AINCOM, .idac_pin = 11, .mux_channel = 9,  .action_id = SENSOR_SHELF1_TEMP },
    { .channel_id = 0, .input_positive = 2,  .input_negative = ADS_AINCOM, .idac_pin = 11, .mux_channel = 10, .action_id = SENSOR_SHELF1_SAMPLE_TEMP },
};

static const struct rtd_config *active_rtds = NULL;
static uint8_t num_active_rtds = 0;
const struct device *ads_dev = DEVICE_DT_GET_ANY(ti_ads124s08);

static void assign_temp_to_msg(struct sys_temp_msg *msg, uint8_t action_id, int16_t temp_dC)
{
    switch (action_id)
    {
        case SENSOR_COLLECTOR_TEMP:                          msg->collector_dC         = temp_dC; break;
        case SENSOR_AMBIENT_TEMPERATURE:                     msg->ambient_dC           = temp_dC; break;
        case SENSOR_FIRST_STAGE_HEAT_EXCHANGER_OUTLET_TEMP:  msg->hx_1st_stage_dC      = temp_dC; break;
        case SENSOR_SECOND_STAGE_HEAT_EXCHANGER_OUTLET_TEMP: msg->hx_2nd_stage_dC      = temp_dC; break;
        case SENSOR_FIRST_STAGE_SUCTION_TEMP:                msg->suction_1st_stage_dC = temp_dC; break;
        case SENSOR_SECOND_STAGE_SUCTION_TEMP:               msg->suction_2nd_stage_dC = temp_dC; break;
        case SENSOR_SHELL_FREEZER_TEMP_SENSOR:
        case SENSOR_MINI_CHAMBER_TEMP:                       msg->shell_or_mini_dC     = temp_dC; break;
        case SENSOR_LEXSOL_TEMP:                             msg->triad_lexsol_dC      = temp_dC; break;
        case SENSOR_SHELF1_TEMP:                             msg->triad_shelf_dC       = temp_dC; break;
        case SENSOR_SHELF1_SAMPLE_TEMP:                      msg->triad_sample_dC      = temp_dC; break;
        default: break;
    }
}

static void set_rtd_mux_channel(uint8_t address)
{
    gpio_pin_set_dt(&mux_en1, 0);
    gpio_pin_set_dt(&mux_en2, 0);

    gpio_pin_set_dt(&mux_a0, (address >> 0) & 1);
    gpio_pin_set_dt(&mux_a1, (address >> 1) & 1);
    gpio_pin_set_dt(&mux_a2, (address >> 2) & 1);

    if (address < 8)
    {
        gpio_pin_set_dt(&mux_en1, 1);
    }
    else
    {
        gpio_pin_set_dt(&mux_en2, 1);
    }

    k_msleep(10);
}

static float adc_to_resistance(int32_t adc)
{
    return ((float)adc / ADC_FS) * RREF;
}

static float resistance_to_temperature(float resistance)
{
    return (-RTD_A + sqrtf(RTD_A * RTD_A - 4 * RTD_B * (1 - resistance / RTD_R0))) / (2 * RTD_B);
}

static void read_hardware_temperatures(struct sys_temp_msg *msg)
{
    #define NUM_SAMPLES 5
    int err;
    int32_t adc_samples[NUM_SAMPLES];

    for (int i = 0; i < num_active_rtds; i++)
    {
        set_rtd_mux_channel(active_rtds[i].mux_channel);
        k_msleep(10);

        struct adc_channel_cfg cfg =
        {
            .channel_id             = 0,
            .differential           = true,
            .input_positive         = active_rtds[i].input_positive,
            .input_negative         = active_rtds[i].input_negative,
            .gain                   = ADC_GAIN_1,
            .reference              = ADC_REF_EXTERNAL0,
            .acquisition_time       = ADC_ACQ_TIME_DEFAULT,
            .current_source_pin_set = true,
            .current_source_pin     = { 11, 15 },
        };

        err = adc_channel_setup(ads_dev, &cfg);
        k_msleep(10);
        if (err != 0)
        {
            LOG_ERR("ADC setup failed for RTD%d: %d", i + 1, err);
            continue;
        }

        struct adc_sequence sequence =
        {
            .channels     = BIT(0),
            .buffer       = &adc_samples[0],
            .buffer_size  = sizeof(adc_samples[0]),
            .resolution   = 24,
            .oversampling = 0,
            .calibrate    = false,
        };

        float temperature_sum = 0.0f;
        int valid_samples = 0;

        for (int sample = 0; sample < NUM_SAMPLES; sample++)
        {
            sequence.buffer = &adc_samples[sample];
            err = adc_read(ads_dev, &sequence);
            if (err != 0)
            {
                LOG_ERR("ADC read failed RTD%d Sample%d: %d", i + 1, sample + 1, err);
                continue;
            }

            float resistance  = adc_to_resistance(adc_samples[sample]);
            temperature_sum  += resistance_to_temperature(resistance);
            valid_samples++;
            k_msleep(20);
        }

        if (valid_samples > 0)
        {
            int16_t avg_dC = (int16_t)((temperature_sum / valid_samples) * 10.0f);
            assign_temp_to_msg(msg, active_rtds[i].action_id, avg_dC);
        }
    }
}
#endif

int ads_task_init(void)
{
    gpio_pin_configure_dt(&mux_a0, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&mux_a1, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&mux_a2, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&mux_en1, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&mux_en2, GPIO_OUTPUT_ACTIVE);

#ifndef SIMULATION
    active_rtds     = max_configs;
    num_active_rtds = ARRAY_SIZE(max_configs);

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
    struct sys_temp_msg temp_msg;
    init_sys_temp_msg(&temp_msg);

#ifdef SIMULATION
    simulate_temperatures(&temp_msg);
#else
    read_hardware_temperatures(&temp_msg);
#endif

    temp_msg.timestamp_ms = k_uptime_get_32();
    int err = zbus_chan_pub(&ZBUS_CHAN_SYSTEM_TEMPERATURE, &temp_msg, K_MSEC(5));
    if (err != 0)
    {
        LOG_ERR("[ZBUS PUB] failed: %d", err);
    }
}
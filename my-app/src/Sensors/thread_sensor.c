#include "build_config.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "thread_sensor.h"
#include "ads124s08.h"
#include "boot_sequence.h"
#include "product_type.h"
#include "main_iob_data.h"
#include "protocol_defs.h"
#include "sensor_types.h"
#include "zbus_channels.h"

LOG_MODULE_REGISTER(thread_sensor, LOG_LEVEL_INF);



extern struct k_mutex shadow_memory_mutex;
/* =========================================================
 * 1. EVERY 50 ms: VACUUM SENSORS (I2C Bus)
 * ========================================================= */
static void vacuum_sensors_50ms_run(void)
{
    struct sys_vac_msg vac_msg =
    {
        .system_vac_mbar    = 0xFFFF,
        .sample_vac_mbar    = { 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF },
        .epd_addon_vac_1_mbar = 0xFFFF,
        .epd_addon_vac_2_mbar = 0xFFFF,
        .timestamp_ms       = k_uptime_get_32(),
        .valid              = false,
    };

#ifdef SIMULATION
    static uint16_t sim_vac_mbar = 1000;
    sim_vac_mbar = (sim_vac_mbar <= 100) ? 1000 : (uint16_t)(sim_vac_mbar - 5);

    vac_msg.system_vac_mbar = sim_vac_mbar;
    for (int i = 0; i < 6; i++)
    {
        vac_msg.sample_vac_mbar[i] = (uint16_t)(sim_vac_mbar + (i * 10));
    }
    vac_msg.epd_addon_vac_1_mbar = sim_vac_mbar;
    vac_msg.epd_addon_vac_2_mbar = (uint16_t)(sim_vac_mbar + 5);
    vac_msg.valid = true;

    zbus_chan_pub(&ZBUS_CHAN_SYSTEM_VACUUM, &vac_msg, K_MSEC(5));
#else
    /* TODO: Read physical I2C Vacuum Sensor MUX & Transducers (50 ms) */
    (void)vac_msg;
#endif
}


/* =========================================================
 * 2. EVERY 100 ms: DIGITAL SENSORS (Tilt Switch, Moisture, ZCD)
 * ========================================================= */
static void digital_sensors_100ms_run(void)
{
#ifdef SIMULATION
    k_mutex_lock(&shadow_memory_mutex, K_FOREVER);
    if (u8_ProductID == IAM_TRIAD)
    {
        /* Simulate Platen Tilt Switch = OPEN (0 = Level / Safe) */
        uint8_t sim_tilt = SWITCH_OPEN;
        MainIOBShadowMemory.TRIAD.Digital_Sensors.TiltSwitch = sim_tilt;
        MainIOBShadowMemory.TRIAD.Digital_Sensors.Last_Update_ms = k_uptime_get_32();
        LOG_INF("[SIM] TRIAD TILT SWITCH: %s (%u)",
                sim_tilt == SWITCH_OPEN ? "OPEN" : "CLOSED",
                sim_tilt);
    }
    else if (u8_ProductID == IAM_FD)
    {
        /* Simulate Collector Moisture = NO MOISTURE (0 = Dry) */
        uint8_t sim_moisture = MOISTURE_NO;
        MainIOBShadowMemory.FD.Digital_Sensors.Moisture = sim_moisture;
        MainIOBShadowMemory.FD.Digital_Sensors.Last_Update_ms = k_uptime_get_32();
        LOG_INF("[SIM] FD MOISTURE: %s (%u)",
                sim_moisture == MOISTURE_NO ? "DRY" : "WET",
                sim_moisture);
    }
    k_mutex_unlock(&shadow_memory_mutex);
#else
    /* TODO: Read physical Tilt Switch GPIO & Moisture GPIO */
#endif
}

/* =========================================================
 * 3. EVERY 1000 ms: SLOW SYSTEM BUS (PMB, RTC, EEPROM)
 * ========================================================= */
static void slow_bus_1000ms_run(void)
{
#ifdef SIMULATION
    /* Nothing required in simulation */
#else
    /* TODO: Read Power Monitor (PMB), RTC Time/Date & EEPROM sync */
    // FD 3.0 no PMB
#endif
}

/* =========================================================
 * THREAD 4 ENTRY POINTS (50 ms Base Period)
 * ========================================================= */
int sensor_task_init(void)
{
    int ret = ads_task_init();
    if (ret < 0) {
        LOG_ERR("ADS124S08 init failed (%d)", ret);
        return ret;
    }

    LOG_INF("4_Sensor_Thread initialized (50ms / 100ms / 1000ms dispatcher)");
    return 0;
}

void sensor_task_run(void)
{
    static uint8_t tick_50ms = 0;
    tick_50ms++;

    /* 1. Every 50 ms (Every Tick): RTD ADC Stepper + Vacuum Sensors */
    ads_task_run();
    vacuum_sensors_50ms_run();

    /* 2. Every 100 ms (Every 2nd Tick): Tilt Switch, Moisture, AC Zero-Cross */
    if ((tick_50ms % 2U) == 0U) 
    {
        digital_sensors_100ms_run();
    }

    /* 3. Every 1000 ms (Every 20th Tick): Power Monitor, RTC & EEPROM */
    if (tick_50ms >= 20U) 
    {
        tick_50ms = 0U;
        slow_bus_1000ms_run();
    }
}
#include "build_config.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "task_sensor.h"
#include "task_ads124s08.h"
#include "boot_sequence.h"
#include "product_type.h"
#include "main_iob_data.h"
#include "protocol_defs.h"
#include "sensor_types.h"

LOG_MODULE_REGISTER(task_sensor, LOG_LEVEL_INF);

/* =========================================================
 * 1. EVERY 50 ms: VACUUM SENSORS (I2C Bus)
 * ========================================================= */
static void vacuum_sensors_50ms_run(void)
{
#ifdef SIMULATION

    // This simulation to be done only when APP do a set pont. otherwise environment vac point to be present
    /* Simulate System Vacuum ramping down from 1000 mBar to 100 mBar */
    static int16_t sim_vac_mtorr = 1000;
    sim_vac_mtorr -= 5;
    if (sim_vac_mtorr < 100) {
        sim_vac_mtorr = 1000;
    }

    VacuumStatus_t vac_pkt;
    vac_pkt.state.vacuum   = sim_vac_mtorr;
    vac_pkt.state.status   = 0x01; /* Sensor Present & OK */
    vac_pkt.state.reserved = 0x00;

    UpdateMainIOBData(SENSOR_SYSTEM_VAC, vac_pkt.raw);
    LOG_INF("[SIM] SYSTEM VACUUM: %d mBar | Status: 0x%02X",
        sim_vac_mtorr,
        vac_pkt.state.status);
    

#else
    /* TODO: Read physical I2C Vacuum Sensor MUX & Transducers (50 ms) */


#endif




}

/* =========================================================
 * 2. EVERY 100 ms: DIGITAL SENSORS (Tilt Switch, Moisture, ZCD)
 * ========================================================= */
static void digital_sensors_100ms_run(void)
{
#ifdef SIMULATION
    if (u8_ProductID == IAM_TRIAD) 
    {
        /* Simulate Platen Tilt Switch = OPEN (0 = Level / Safe) */
        uint8_t sim_tilt = SWITCH_OPEN;
        UpdateMainIOBData(SENSOR_TILT_SW, &sim_tilt);
        LOG_INF("[SIM] TRIAD TILT SWITCH: %s (%u)",
        sim_tilt == SWITCH_OPEN ? "OPEN" : "CLOSED",
        sim_tilt);
    } 
    else if (u8_ProductID == IAM_FD) 
    {
        /* Simulate Collector Moisture = NO MOISTURE (0 = Dry) */
        uint8_t sim_moisture = MOISTURE_NO;
        UpdateMainIOBData(SENSOR_MOISTURE, &sim_moisture);
        LOG_INF("[SIM] FD MOISTURE: %s (%u)",
        sim_moisture == MOISTURE_NO ? "DRY" : "WET",
        sim_moisture);
    }
#else
    /* TODO: Read physical Tilt Switch GPIO, Moisture GPIO & AC Zero-Cross */
    // No AC ZCD in FD 3.0
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
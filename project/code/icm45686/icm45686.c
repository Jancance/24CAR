#include "icm45686.h"
#include "icm45686_port.h"
#include "vendor/inv_imu_driver.h"

#include <stddef.h>
#include <string.h>

#define ICM45686_ACCEL_RANGE_G   (4.0f)
#define ICM45686_GYRO_RANGE_DPS  (1000.0f)

static inv_imu_device_t icm_device;
static icm45686_data_t icm_data;
static icm45686_attitude_t icm_attitude;
static uint8 icm_who_am_i;
static int icm_last_error;
static uint8 icm_initialized;
static uint32 icm_last_update_ms;

static int icm45686_configure(void)
{
    int result = 0;
    inv_imu_int_pin_config_t pin_config = { 0 };
    inv_imu_int_state_t interrupt_config;
    drive_config0_t drive_config = { 0 };

    drive_config.pads_spi_slew = DRIVE_CONFIG0_PADS_SPI_SLEW_TYP_10NS;
    result |= inv_imu_write_reg(&icm_device,
                                DRIVE_CONFIG0,
                                1U,
                                (uint8_t *)&drive_config);
    icm45686_port_delay_us(2U);

    result |= inv_imu_get_who_am_i(&icm_device, &icm_who_am_i);
    if ((result != 0) || (icm_who_am_i != INV_IMU_WHOAMI))
    {
        return (result != 0) ? result : -2;
    }

    result |= inv_imu_soft_reset(&icm_device);

    pin_config.int_polarity = INTX_CONFIG2_INTX_POLARITY_HIGH;
    pin_config.int_mode = INTX_CONFIG2_INTX_MODE_PULSE;
    pin_config.int_drive = INTX_CONFIG2_INTX_DRIVE_PP;
    result |= inv_imu_set_pin_config_int(&icm_device, INV_IMU_INT1, &pin_config);

    memset(&interrupt_config, INV_IMU_DISABLE, sizeof(interrupt_config));
    interrupt_config.INV_UI_DRDY = INV_IMU_ENABLE;
    result |= inv_imu_set_config_int(&icm_device, INV_IMU_INT1, &interrupt_config);

    result |= inv_imu_set_accel_fsr(&icm_device, ACCEL_CONFIG0_ACCEL_UI_FS_SEL_4_G);
    result |= inv_imu_set_gyro_fsr(&icm_device, GYRO_CONFIG0_GYRO_UI_FS_SEL_1000_DPS);
    result |= inv_imu_set_accel_frequency(&icm_device, ACCEL_CONFIG0_ACCEL_ODR_200_HZ);
    result |= inv_imu_set_gyro_frequency(&icm_device, GYRO_CONFIG0_GYRO_ODR_200_HZ);
    result |= inv_imu_set_accel_ln_bw(&icm_device, IPREG_SYS2_REG_131_ACCEL_UI_LPFBW_DIV_4);
    result |= inv_imu_set_gyro_ln_bw(&icm_device, IPREG_SYS1_REG_172_GYRO_UI_LPFBW_DIV_4);
    result |= inv_imu_set_accel_mode(&icm_device, PWR_MGMT0_ACCEL_MODE_LN);
    result |= inv_imu_set_gyro_mode(&icm_device, PWR_MGMT0_GYRO_MODE_LN);

    return result;
}

uint8 icm45686_init(void)
{
    memset(&icm_device, 0, sizeof(icm_device));
    memset(&icm_data, 0, sizeof(icm_data));
    memset(&icm_attitude, 0, sizeof(icm_attitude));
    icm_who_am_i = 0U;
    icm_initialized = 0U;

    icm45686_port_init();
    icm_device.transport.read_reg = icm45686_port_read;
    icm_device.transport.write_reg = icm45686_port_write;
    icm_device.transport.serif_type = UI_SPI4;
    icm_device.transport.sleep_us = icm45686_port_delay_us;

    icm45686_port_delay_us(3000U);
    icm_last_error = icm45686_configure();
    if (icm_last_error != 0)
    {
        return 0U;
    }

    icm45686_attitude_init(200.0f);
    icm_last_update_ms = icm45686_port_get_ms();
    icm_initialized = 1U;
    return 1U;
}

uint8 icm45686_update(void)
{
    inv_imu_sensor_data_t raw;
    uint32 now_ms;
    uint32 elapsed_ms;

    if (!icm_initialized)
    {
        return 0U;
    }

    icm_last_error = inv_imu_get_register_data(&icm_device, &raw);
    if (icm_last_error != 0)
    {
        return 0U;
    }

    icm_data.accel_g[0] = (float)raw.accel_data[0] * ICM45686_ACCEL_RANGE_G / 32768.0f;
    icm_data.accel_g[1] = (float)raw.accel_data[1] * ICM45686_ACCEL_RANGE_G / 32768.0f;
    icm_data.accel_g[2] = (float)raw.accel_data[2] * ICM45686_ACCEL_RANGE_G / 32768.0f;
    icm_data.gyro_dps[0] = (float)raw.gyro_data[0] * ICM45686_GYRO_RANGE_DPS / 32768.0f;
    icm_data.gyro_dps[1] = (float)raw.gyro_data[1] * ICM45686_GYRO_RANGE_DPS / 32768.0f;
    icm_data.gyro_dps[2] = (float)raw.gyro_data[2] * ICM45686_GYRO_RANGE_DPS / 32768.0f;
    icm_data.temperature_c = 25.0f + ((float)raw.temp_data / 128.0f);

    now_ms = icm45686_port_get_ms();
    elapsed_ms = now_ms - icm_last_update_ms;
    icm_last_update_ms = now_ms;
    icm45686_attitude_update(&icm_data, (float)elapsed_ms * 0.001f);
    icm45686_attitude_get(&icm_attitude);
    return 1U;
}

void icm45686_get_data(icm45686_data_t *data)
{
    if (data != NULL)
    {
        *data = icm_data;
    }
}

void icm45686_get_attitude(icm45686_attitude_t *attitude)
{
    if (attitude != NULL)
    {
        *attitude = icm_attitude;
    }
}

void icm45686_reset_yaw(void)
{
    icm45686_attitude_reset_yaw();
}

void icm45686_restart_calibration(void)
{
    icm45686_attitude_restart_calibration();
}

uint8 icm45686_get_who_am_i(void)
{
    return icm_who_am_i;
}

int icm45686_get_last_error(void)
{
    return icm_last_error;
}

#include "icm45686_telemetry.h"
#include "debug_uart.h"

static void telemetry_write_fixed_2(float value)
{
    int32 scaled;
    uint32 magnitude;
    char fraction[3];

    scaled = (int32)((value >= 0.0f) ? (value * 100.0f + 0.5f) : (value * 100.0f - 0.5f));
    if (scaled < 0)
    {
        debug_uart_write_string("-");
        magnitude = (uint32)(-scaled);
    }
    else
    {
        magnitude = (uint32)scaled;
    }

    debug_uart_write_u32(magnitude / 100U);
    debug_uart_write_string(".");
    fraction[0] = (char)('0' + ((magnitude / 10U) % 10U));
    fraction[1] = (char)('0' + (magnitude % 10U));
    fraction[2] = '\0';
    debug_uart_write_string(fraction);
}

static void telemetry_write_value(const char *name, float value)
{
    debug_uart_write_string(",");
    debug_uart_write_string(name);
    debug_uart_write_string("=");
    telemetry_write_fixed_2(value);
}

void icm45686_telemetry_send(uint32 timestamp_ms)
{
    icm45686_data_t data;
    icm45686_attitude_t attitude;
    int error;

    icm45686_get_data(&data);
    icm45686_get_attitude(&attitude);
    error = icm45686_get_last_error();

    debug_uart_write_string("$ICM,ms=");
    debug_uart_write_u32(timestamp_ms);
    debug_uart_write_string(",cal=");
    debug_uart_write_u32(attitude.calibrated);
    debug_uart_write_string(",still=");
    debug_uart_write_u32(attitude.stationary);
    debug_uart_write_string(",who=");
    debug_uart_write_u32(icm45686_get_who_am_i());
    telemetry_write_value("ax", data.accel_g[0]);
    telemetry_write_value("ay", data.accel_g[1]);
    telemetry_write_value("az", data.accel_g[2]);
    telemetry_write_value("gx", data.gyro_dps[0]);
    telemetry_write_value("gy", data.gyro_dps[1]);
    telemetry_write_value("gz", data.gyro_dps[2]);
    telemetry_write_value("yaw", attitude.yaw);
    telemetry_write_value("pitch", attitude.pitch);
    telemetry_write_value("roll", attitude.roll);
    telemetry_write_value("temp", data.temperature_c);
    debug_uart_write_string(",err=");
    if (error < 0)
    {
        debug_uart_write_string("-");
        error = -error;
    }
    debug_uart_write_u32((uint32)error);
    debug_uart_write_string("\r\n");
}

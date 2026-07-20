#include "icm45686_telemetry.h"
#include "Serial.h"

static void telemetry_write_u32(uint32 value)
{
    char text[10];
    uint8 count = 0U;

    if (value == 0U)
    {
        Serial_SendByte('0');
        return;
    }

    while ((value > 0U) && (count < sizeof(text)))
    {
        text[count++] = (char)('0' + (value % 10U));
        value /= 10U;
    }
    while (count > 0U)
    {
        Serial_SendByte((uint8)text[--count]);
    }
}

static void telemetry_write_fixed_2(float value)
{
    int32 scaled;
    uint32 magnitude;
    char fraction[3];

    scaled = (int32)((value >= 0.0f) ? (value * 100.0f + 0.5f) : (value * 100.0f - 0.5f));
    if (scaled < 0)
    {
        Serial_SendString("-");
        magnitude = (uint32)(-scaled);
    }
    else
    {
        magnitude = (uint32)scaled;
    }

    telemetry_write_u32(magnitude / 100U);
    Serial_SendString(".");
    fraction[0] = (char)('0' + ((magnitude / 10U) % 10U));
    fraction[1] = (char)('0' + (magnitude % 10U));
    fraction[2] = '\0';
    Serial_SendString(fraction);
}

static void telemetry_write_value(const char *name, float value)
{
    Serial_SendString(",");
    Serial_SendString(name);
    Serial_SendString("=");
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

    Serial_SendString("$ICM,ms=");
    telemetry_write_u32(timestamp_ms);
    Serial_SendString(",cal=");
    telemetry_write_u32(attitude.calibrated);
    Serial_SendString(",still=");
    telemetry_write_u32(attitude.stationary);
    Serial_SendString(",who=");
    telemetry_write_u32(icm45686_get_who_am_i());
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
    Serial_SendString(",err=");
    if (error < 0)
    {
        Serial_SendString("-");
        error = -error;
    }
    telemetry_write_u32((uint32)error);
    Serial_SendString("\r\n");
}

#include "icm45686_page.h"
#include "OLED.h"

static void icm45686_page_show_signed(uint8 x, uint8 y, float value)
{
    uint32 magnitude;

    if (value < 0.0f)
    {
        OLED_ShowChar(x, y, '-', 8U);
        value = -value;
    }
    else
    {
        OLED_ShowChar(x, y, '+', 8U);
    }

    magnitude = (uint32)(value * 100.0f + 0.5f);
    if (magnitude > 99999U)
    {
        magnitude = 99999U;
    }

    OLED_ShowNum((uint8)(x + 6U), y, magnitude / 100U, 3U, 8U);
    OLED_ShowChar((uint8)(x + 24U), y, '.', 8U);
    OLED_ShowNum((uint8)(x + 30U), y, (magnitude / 10U) % 10U, 1U, 8U);
    OLED_ShowNum((uint8)(x + 36U), y, magnitude % 10U, 1U, 8U);
}

void icm45686_page_init(uint8 init_ok)
{
    OLED_Clear();
    OLED_ShowString(0U, 0U, (uint8_t *)"ICM45686", 8U);
    if (init_ok)
    {
        OLED_ShowString(54U, 0U, (uint8_t *)"OK", 8U);
    }
    else
    {
        OLED_ShowString(54U, 0U, (uint8_t *)"ERR", 8U);
        OLED_ShowString(0U, 2U, (uint8_t *)"WHO:", 8U);
        OLED_ShowNum(30U, 2U, icm45686_get_who_am_i(), 3U, 8U);
        OLED_ShowString(0U, 3U, (uint8_t *)"CODE:", 8U);
        OLED_ShowNum(36U,
                     3U,
                     (uint32)((icm45686_get_last_error() < 0) ?
                                  -icm45686_get_last_error() :
                                  icm45686_get_last_error()),
                     3U,
                     8U);
    }
}

void icm45686_page_refresh(void)
{
    icm45686_attitude_t attitude;
    icm45686_data_t data;

    icm45686_get_attitude(&attitude);
    icm45686_get_data(&data);

    OLED_ShowString(0U, 1U, (uint8_t *)"Y:", 8U);
    icm45686_page_show_signed(12U, 1U, attitude.yaw);
    OLED_ShowString(0U, 2U, (uint8_t *)"P:", 8U);
    icm45686_page_show_signed(12U, 2U, attitude.pitch);
    OLED_ShowString(0U, 3U, (uint8_t *)"R:", 8U);
    icm45686_page_show_signed(12U, 3U, attitude.roll);
    OLED_ShowString(0U, 4U, (uint8_t *)"GZ:", 8U);
    icm45686_page_show_signed(18U, 4U, data.gyro_dps[2]);
    OLED_ShowString(0U, 5U, (uint8_t *)"CAL:", 8U);
    OLED_ShowString(30U,
                    5U,
                    attitude.calibrated ?
                        (attitude.stationary ? (uint8_t *)"LOCK" : (uint8_t *)"OK  ") :
                        (uint8_t *)"WAIT",
                    8U);
}

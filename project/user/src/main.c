#include "zf_common_headfile.h"
#include "board_pins.h"
#include "OLED.h"

#define GRAY_DISPLAY_PERIOD_MS (50U)

static void oled_show_gray_array(void)
{
    uint8 index;

    for (index = 0U; index < CAR_GRAY_COUNT; index++)
    {
        OLED_ShowNum((uint8)(24U + index * 12U), 2U,
                     (uint32)gray_values[index], 1U, 8U);
    }
}

int main(void)
{
    uint8 raw_mask;
    uint8 active_mask;
    uint32 display_ms;
    uint32 now_ms;

    clock_init(SYSTEM_CLOCK_80M);
    OLED_Init();
    OLED_Clear();
    Key_Init();
    gray_init();
    system_pit_init();

    OLED_ShowString(0U, 0U, (uint8 *)"GRAY ARRAY TEST", 8U);
    OLED_ShowString(0U, 1U, (uint8 *)"CH: 1 2 3 4 5 6 7 8", 8U);
    OLED_ShowString(0U, 2U, (uint8 *)"S:", 8U);
    OLED_ShowString(0U, 3U, (uint8 *)"RAW DEC:", 8U);
    OLED_ShowString(0U, 4U, (uint8 *)"ACT DEC:", 8U);
    OLED_ShowString(0U, 5U, (uint8 *)"BLACK=1 WHITE=0", 8U);
    OLED_ShowString(0U, 6U, (uint8 *)"ACTIVE LEVEL:LOW", 8U);
    OLED_ShowString(0U, 7U, (uint8 *)"MOVE BLACK LINE", 8U);

    display_ms = system_get_ms() - GRAY_DISPLAY_PERIOD_MS;
    while (true)
    {
        gray_update();
        now_ms = system_get_ms();
        if ((uint32)(now_ms - display_ms) >= GRAY_DISPLAY_PERIOD_MS)
        {
            display_ms = now_ms;
            raw_mask = gray_read_raw();
            active_mask = gray_read_active();
            oled_show_gray_array();
            OLED_ShowNum(54U, 3U, (uint32)raw_mask, 3U, 8U);
            OLED_ShowNum(54U, 4U, (uint32)active_mask, 3U, 8U);
        }
    }
}

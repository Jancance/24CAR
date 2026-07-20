#include "zf_common_headfile.h"
#include "board_pins.h"
#include "OLED.h"

int main(void)
{
    clock_init(SYSTEM_CLOCK_80M);
    ALL_Init();

    OLED_Clear();

    while (true)
    {
        OLED_ShowString(0U, 0U, (uint8 *)"Hello World", 8U);
    }
}

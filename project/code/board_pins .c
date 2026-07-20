#include "board_pins.h"

void ALL_Init(void)
{
    clock_init(SYSTEM_CLOCK_80M);
    OLED_Init();
    OLED_Clear();
    led_init();
    buzzer_init();
    Key_Init();
    motor_init();
    car_encoder_init();
    gray_init();
    system_pit_init();
    Serial_Init();
    speed_control_init();
}

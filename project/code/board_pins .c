#include "board_pins.h"

void ALL_Init(void)
{
    OLED_Init();
    OLED_Clear();
    led_init();
    buzzer_init();
    Key_Init();
    motor_init();
    car_encoder_init();
    gray_init();
    system_pit_init();
    debug_uart_init(CAR_DEBUG_UART_BAUD);
}

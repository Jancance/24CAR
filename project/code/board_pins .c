 
#include "board_pins.h"

static volatile uint32 system_ms;



void pit_handler (uint32 state, void *ptr)  //作为系统的更新中断，1ms
{
    (void)state;
    (void)ptr;
    system_ms++;
    Key_Tick();
}


void system_pit_init (void)
{
    system_ms = 0U;
    pit_ms_init(CAR_SYSTEM_PIT, 1, pit_handler, NULL);
    interrupt_set_priority(CAR_SYSTEM_PIT_IRQn, 0);
}

uint32 system_get_ms(void)
{
    return system_ms;
}


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

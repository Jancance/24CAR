#include "buzzer.h"
#include "board_pins.h"

void buzzer_init(void)
{
    gpio_init(CAR_BUZZER_PIN,
              GPO,
              (CAR_BUZZER_ACTIVE_LEVEL == GPIO_HIGH) ? GPIO_LOW : GPIO_HIGH,
              GPO_PUSH_PULL);
}

void buzzer_on(void)
{
    gpio_set_level(CAR_BUZZER_PIN, CAR_BUZZER_ACTIVE_LEVEL);
}

void buzzer_off(void)
{
    gpio_set_level(CAR_BUZZER_PIN,
                   (CAR_BUZZER_ACTIVE_LEVEL == GPIO_HIGH) ? GPIO_LOW : GPIO_HIGH);
}

void buzzer_toggle(void)
{
    gpio_toggle_level(CAR_BUZZER_PIN);
}

void buzzer_beep(uint16 ms)
{
    buzzer_on();
    system_delay_ms(ms);
    buzzer_off();
	system_delay_ms(ms);
}



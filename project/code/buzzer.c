#include "buzzer.h"
#include "board_pins.h"

static uint8 buzzer_beep_active;
static uint32 buzzer_beep_start_ms;
static uint16 buzzer_beep_duration_ms;

void buzzer_init(void)
{
    gpio_init(CAR_BUZZER_PIN,
              GPO,
              (CAR_BUZZER_ACTIVE_LEVEL == GPIO_HIGH) ? GPIO_LOW : GPIO_HIGH,
              GPO_PUSH_PULL);
    buzzer_beep_active = 0U;
    buzzer_beep_start_ms = system_get_ms();
    buzzer_beep_duration_ms = 0U;
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

void buzzer_beep_start(uint16 ms)
{
    buzzer_beep_start_ms = system_get_ms();
    buzzer_beep_duration_ms = ms;
    buzzer_beep_active = 1U;
    buzzer_on();
}

void buzzer_loop(void)
{
    if (buzzer_beep_active &&
        (uint32)(system_get_ms() - buzzer_beep_start_ms) >=
            buzzer_beep_duration_ms)
    {
        buzzer_beep_active = 0U;
        buzzer_off();
    }
}



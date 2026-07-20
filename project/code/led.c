#include "led.h"
#include "board_pins.h"

static const gpio_pin_enum status_led_pins[CAR_STATUS_LED_COUNT] = {
    CAR_STATUS_LED1_PIN,
    CAR_STATUS_LED2_PIN,
};

static const gpio_pin_enum rgb_led_pins[CAR_RGB_LED_COUNT] = {
    CAR_RGB_LED1_PIN,
    CAR_RGB_LED2_PIN,
    CAR_RGB_LED3_PIN,
};

void led_init(void)
{
    uint8 i;

    for (i = 0; i < CAR_STATUS_LED_COUNT; i++)
    {
        gpio_init(status_led_pins[i], GPO, GPIO_LOW, GPO_PUSH_PULL);
    }

    for (i = 0; i < CAR_RGB_LED_COUNT; i++)
    {
        gpio_init(rgb_led_pins[i], GPO, GPIO_HIGH, GPO_PUSH_PULL);
    }
}

void led_set(uint8 index, uint8 state)
{
    if (index >= CAR_STATUS_LED_COUNT)
    {
        return;
    }

    gpio_set_level(status_led_pins[index], state ? GPIO_HIGH : GPIO_LOW);
}

void led_toggle(uint8 index)
{
    if (index >= CAR_STATUS_LED_COUNT)
    {
        return;
    }

    gpio_toggle_level(status_led_pins[index]);
}

void rgb_led_set(uint8 index, uint8 state)
{
    if (index >= CAR_RGB_LED_COUNT)
    {
        return;
    }

    gpio_set_level(rgb_led_pins[index], state ? GPIO_LOW : GPIO_HIGH);
}

void rgb_led_toggle(uint8 index)
{
    if (index >= CAR_RGB_LED_COUNT)
    {
        return;
    }

    gpio_toggle_level(rgb_led_pins[index]);
}

#include "gray.h"
#include "board_pins.h"

static const gpio_pin_enum gray_pins[CAR_GRAY_COUNT] = {
    CAR_GRAY_1_PIN,
    CAR_GRAY_2_PIN,
    CAR_GRAY_3_PIN,
    CAR_GRAY_4_PIN,
    CAR_GRAY_5_PIN,
    CAR_GRAY_6_PIN,
    CAR_GRAY_7_PIN,
    CAR_GRAY_8_PIN,
};

void gray_init(void)
{
    uint8 index;

    for (index = 0U; index < CAR_GRAY_COUNT; index++)
    {
        gpio_init(gray_pins[index], GPI, GPIO_HIGH, GPI_PULL_UP);
    }
}

uint8 gray_read_channel(uint8 index)
{
    if (index >= CAR_GRAY_COUNT)
    {
        return GPIO_HIGH;
    }

    return gpio_get_level(gray_pins[index]);
}

uint8 gray_read_raw(void)
{
    uint8 index;
    uint8 value = 0U;

    for (index = 0U; index < CAR_GRAY_COUNT; index++)
    {
        if (gpio_get_level(gray_pins[index]) == GPIO_HIGH)
        {
            value |= (uint8)(1U << index);
        }
    }

    return value;
}

uint8 gray_read_active(void)
{
    uint8 raw = gray_read_raw();

    if (CAR_GRAY_ACTIVE_LEVEL == GPIO_LOW)
    {
        return (uint8)(~raw);
    }

    return raw;
}

#include "encoder.h"
#include "board_pins.h"

#include <stddef.h>

static volatile int32 left_total_count;
static volatile int32 right_total_count;
static volatile int32 left_delta_count;
static volatile int32 right_delta_count;

static int32 encoder_get_step(gpio_pin_enum phase_b_pin, uint8 reversed)
{
    int32 step = gpio_get_level(phase_b_pin) ? 1 : -1;
    return reversed ? -step : step;
}

static void encoder_left_callback(uint32 state, void *ptr)
{
    int32 step;

    (void)state;
    (void)ptr;
    step = encoder_get_step(CAR_ENCODER_LEFT_B_PIN, CAR_ENCODER_LEFT_REVERSED);
    left_total_count += step;
    left_delta_count += step;
}

static void encoder_right_callback(uint32 state, void *ptr)
{
    int32 step;

    (void)state;
    (void)ptr;
    step = encoder_get_step(CAR_ENCODER_RIGHT_B_PIN, CAR_ENCODER_RIGHT_REVERSED);
    right_total_count += step;
    right_delta_count += step;
}

void car_encoder_init(void)
{
    left_total_count = 0;
    right_total_count = 0;
    left_delta_count = 0;
    right_delta_count = 0;

    gpio_init(CAR_ENCODER_LEFT_B_PIN, GPI, GPIO_HIGH, GPI_PULL_UP);
    gpio_init(CAR_ENCODER_RIGHT_B_PIN, GPI, GPIO_HIGH, GPI_PULL_UP);
    exti_init(CAR_ENCODER_LEFT_A_PIN, EXTI_TRIGGER_RISING, encoder_left_callback, NULL);
    exti_init(CAR_ENCODER_RIGHT_A_PIN, EXTI_TRIGGER_RISING, encoder_right_callback, NULL);
}

void car_encoder_clear(void)
{
    uint32 primask = interrupt_global_disable();

    left_total_count = 0;
    right_total_count = 0;
    left_delta_count = 0;
    right_delta_count = 0;

    interrupt_global_enable(primask);
}

void car_encoder_get_count(int32 *left_count, int32 *right_count)
{
    uint32 primask = interrupt_global_disable();

    if (left_count != NULL)
    {
        *left_count = left_total_count;
    }
    if (right_count != NULL)
    {
        *right_count = right_total_count;
    }

    interrupt_global_enable(primask);
}

void car_encoder_get_delta(int32 *left_delta, int32 *right_delta)
{
    uint32 primask = interrupt_global_disable();

    if (left_delta != NULL)
    {
        *left_delta = left_delta_count;
    }
    if (right_delta != NULL)
    {
        *right_delta = right_delta_count;
    }
    left_delta_count = 0;
    right_delta_count = 0;

    interrupt_global_enable(primask);
}

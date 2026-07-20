#include "encoder.h"
#include "board_pins.h"

#include <stddef.h>

static volatile int32 left_total_count;
static volatile int32 right_total_count;
static volatile int32 left_delta_count;
static volatile int32 right_delta_count;
static uint8 right_previous_state;

// PA21/PA22 form a real TIMG8 quadrature pair and expose the raw x4 count.
static void encoder_left_update_from_hardware(void)
{
    int32 qei_count = (int32)encoder_get_count(TIM_G8);

    encoder_clear_count(TIM_G8);
    if (CAR_ENCODER_LEFT_REVERSED)
    {
        qei_count = -qei_count;
    }
    left_total_count += qei_count;
    left_delta_count += qei_count;
}

static uint8 encoder_right_get_state(void)
{
    uint8 phase_a = gpio_get_level(CAR_ENCODER_RIGHT_A_PIN) ? 1U : 0U;
    uint8 phase_b = gpio_get_level(CAR_ENCODER_RIGHT_B_PIN) ? 1U : 0U;

    return (uint8)((phase_a << 1U) | phase_b);
}

static void encoder_right_callback(uint32 state, void *ptr)
{
    static const int8 quadrature_step[16] = {
         0,  1, -1,  0,
        -1,  0,  0,  1,
         1,  0,  0, -1,
         0, -1,  1,  0
    };
    uint8 current_state;
    int32 step;

    (void)state;
    (void)ptr;
    current_state = encoder_right_get_state();
    step = quadrature_step[(right_previous_state << 2U) | current_state];
    right_previous_state = current_state;
    if (CAR_ENCODER_RIGHT_REVERSED)
    {
        step = -step;
    }
    right_total_count += step;
    right_delta_count += step;
}

void car_encoder_init(void)
{
    left_total_count = 0;
    right_total_count = 0;
    left_delta_count = 0;
    right_delta_count = 0;

    encoder_quad_init(TIM_G8, TIMG8_ENCODER1_CH1_A21, TIMG8_ENCODER1_CH2_A22);
    encoder_clear_count(TIM_G8);
    exti_init(CAR_ENCODER_RIGHT_A_PIN, EXTI_TRIGGER_BOTH, encoder_right_callback, NULL);
    exti_init(CAR_ENCODER_RIGHT_B_PIN, EXTI_TRIGGER_BOTH, encoder_right_callback, NULL);
    right_previous_state = encoder_right_get_state();
}

void car_encoder_clear(void)
{
    uint32 primask = interrupt_global_disable();

    encoder_clear_count(TIM_G8);
    left_total_count = 0;
    right_total_count = 0;
    left_delta_count = 0;
    right_delta_count = 0;
    right_previous_state = encoder_right_get_state();

    interrupt_global_enable(primask);
}

void car_encoder_get_count(int32 *left_count, int32 *right_count)
{
    uint32 primask = interrupt_global_disable();

    encoder_left_update_from_hardware();
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

    encoder_left_update_from_hardware();
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

float car_encoder_count_to_mm(int32 count)
{
    const float wheel_circumference_mm = 3.1415926f * CAR_WHEEL_DIAMETER_MM;

    return ((float)count * wheel_circumference_mm) /
           (float)CAR_ENCODER_COUNTS_PER_WHEEL_REV;
}

void car_encoder_get_distance_mm(float *left_mm, float *right_mm)
{
    int32 left_count;
    int32 right_count;

    car_encoder_get_count(&left_count, &right_count);
    if (left_mm != NULL)
    {
        *left_mm = car_encoder_count_to_mm(left_count);
    }
    if (right_mm != NULL)
    {
        *right_mm = car_encoder_count_to_mm(right_count);
    }
}

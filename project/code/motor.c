#include "motor.h"
#include "board_pins.h"

static int8 left_direction;
static int8 right_direction;

static int32 motor_limit(int32 command)
{
    if (command > (int32)CAR_MOTOR_MAX_DUTY)
    {
        return (int32)CAR_MOTOR_MAX_DUTY;
    }
    if (command < -(int32)CAR_MOTOR_MAX_DUTY)
    {
        return -(int32)CAR_MOTOR_MAX_DUTY;
    }
    return command;
}

static void motor_apply(pwm_channel_enum pwm_pin,
                        gpio_pin_enum dir_pin,
                        int32 command,
                        int8 *last_direction)
{
    int8 direction = 0;
    uint32 duty = 0U;

    if (command > 0)
    {
        direction = 1;
        duty = (uint32)command;
    }
    else if (command < 0)
    {
        direction = -1;
        duty = (uint32)(-command);
    }

    if (direction == 0)
    {
        pwm_set_duty(pwm_pin, 0U);
        gpio_low(dir_pin);
        *last_direction = 0;
        return;
    }

    if (direction != *last_direction)
    {
        // 换向前先撤掉当前 PWM，避免直接从正转切到反转。
        pwm_set_duty(pwm_pin, 0U);
    }

    if (direction > 0)
    {
        // DRV8870: IN2=0，IN1 输出普通 PWM。
        gpio_low(dir_pin);
    }
    else
    {
        // DRV8870: IN2=1 时需要反相 PWM，低电平区间产生反向驱动。
        gpio_high(dir_pin);
        duty = (uint32)CAR_MOTOR_MAX_DUTY - duty;
    }

    pwm_set_duty(pwm_pin, duty);
    *last_direction = direction;
}

void motor_init(void)
{
    gpio_init(CAR_MOTOR_LEFT_DIR_PIN, GPO, GPIO_LOW, GPO_PUSH_PULL);
    gpio_init(CAR_MOTOR_RIGHT_DIR_PIN, GPO, GPIO_LOW, GPO_PUSH_PULL);

    pwm_init(CAR_MOTOR_LEFT_PWM, CAR_MOTOR_PWM_FREQ, 0U);
    pwm_init(CAR_MOTOR_RIGHT_PWM, CAR_MOTOR_PWM_FREQ, 0U);

    left_direction = 0;
    right_direction = 0;
}

void motor_set(int16 left, int16 right)
{
    int32 left_command = motor_limit((int32)left);
    int32 right_command = motor_limit((int32)right);

    if (CAR_MOTOR_LEFT_REVERSED)
    {
        left_command = -left_command;
    }
    if (CAR_MOTOR_RIGHT_REVERSED)
    {
        right_command = -right_command;
    }

    motor_apply(CAR_MOTOR_LEFT_PWM,
                CAR_MOTOR_LEFT_DIR_PIN,
                left_command,
                &left_direction);
    motor_apply(CAR_MOTOR_RIGHT_PWM,
                CAR_MOTOR_RIGHT_DIR_PIN,
                right_command,
                &right_direction);
}

void motor_stop(void)
{
    motor_set(0, 0);
}

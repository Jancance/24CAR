#include "motor.h"
#include "board_pins.h"

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

static void motor_apply(pwm_channel_enum in1_pwm,
                        pwm_channel_enum in2_pwm,
                        int32 command)
{
    uint32 duty;

    if (command > 0)
    {
        duty = (uint32)command;
        // IN1=PWM、IN2=0；PWM关断阶段滑行。
        pwm_set_duty(in2_pwm, 0U);
        pwm_set_duty(in1_pwm, duty);
    }
    else if (command < 0)
    {
        duty = (uint32)(-command);
        // IN1=0、IN2=PWM；反方向也使用相同的驱动/滑行方式。
        pwm_set_duty(in1_pwm, 0U);
        pwm_set_duty(in2_pwm, duty);
    }
    else
    {
        // 两个输入均低，DRV8870滑行停止/低功耗。
        pwm_set_duty(in1_pwm, 0U);
        pwm_set_duty(in2_pwm, 0U);
    }
}

void motor_init(void)
{
    pwm_init(CAR_MOTOR_LEFT_IN1_PWM, CAR_MOTOR_PWM_FREQ, 0U);
    pwm_init(CAR_MOTOR_LEFT_IN2_PWM, CAR_MOTOR_PWM_FREQ, 0U);
    pwm_init(CAR_MOTOR_RIGHT_IN1_PWM, CAR_MOTOR_PWM_FREQ, 0U);
    pwm_init(CAR_MOTOR_RIGHT_IN2_PWM, CAR_MOTOR_PWM_FREQ, 0U);
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

    motor_apply(CAR_MOTOR_LEFT_IN1_PWM,
                CAR_MOTOR_LEFT_IN2_PWM,
                left_command);
    motor_apply(CAR_MOTOR_RIGHT_IN1_PWM,
                CAR_MOTOR_RIGHT_IN2_PWM,
                right_command);
}

void motor_stop(void)
{
    motor_set(0, 0);
}

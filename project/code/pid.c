#include "pid.h"

#include <stddef.h>

static float pid_clamp(float value, float minimum, float maximum)
{
    if (value > maximum)
    {
        return maximum;
    }
    if (value < minimum)
    {
        return minimum;
    }
    return value;
}

static void pid_order_limits(float *minimum, float *maximum)
{
    float temporary;

    if (*minimum > *maximum)
    {
        temporary = *minimum;
        *minimum = *maximum;
        *maximum = temporary;
    }
}

void pid_init(pid_t *pid,
              float kp, float ki, float kd,
              float output_min, float output_max,
              float integral_min, float integral_max)
{
    if (pid == NULL)
    {
        return;
    }

    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
    pid->output_min = output_min;
    pid->output_max = output_max;
    pid->integral_min = integral_min;
    pid->integral_max = integral_max;
    pid_order_limits(&pid->output_min, &pid->output_max);
    pid_order_limits(&pid->integral_min, &pid->integral_max);
    pid_reset(pid);
}

void pid_reset(pid_t *pid)
{
    if (pid == NULL)
    {
        return;
    }

    pid->target = 0.0f;
    pid->feedback = 0.0f;
    pid->error = 0.0f;
    pid->previous_error = 0.0f;
    pid->integral = 0.0f;
    pid->derivative = 0.0f;
    pid->output = 0.0f;
    pid->initialized = 0U;
}

void pid_set_gains(pid_t *pid, float kp, float ki, float kd)
{
    if (pid == NULL)
    {
        return;
    }

    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
}

void pid_set_limits(pid_t *pid,
                    float output_min, float output_max,
                    float integral_min, float integral_max)
{
    if (pid == NULL)
    {
        return;
    }

    pid->output_min = output_min;
    pid->output_max = output_max;
    pid->integral_min = integral_min;
    pid->integral_max = integral_max;
    pid_order_limits(&pid->output_min, &pid->output_max);
    pid_order_limits(&pid->integral_min, &pid->integral_max);
    pid->integral = pid_clamp(pid->integral,
                              pid->integral_min,
                              pid->integral_max);
    pid->output = pid_clamp(pid->output,
                            pid->output_min,
                            pid->output_max);
}

float pid_calculate(pid_t *pid, float target, float feedback, float dt_s)
{
    if (pid == NULL)
    {
        return 0.0f;
    }

    pid->target = target;
    pid->feedback = feedback;
    pid->error = target - feedback;

    if (dt_s > 0.0f)
    {
        pid->integral += pid->error * dt_s;
        pid->integral = pid_clamp(pid->integral,
                                  pid->integral_min,
                                  pid->integral_max);

        if (pid->initialized)
        {
            pid->derivative =
                (pid->error - pid->previous_error) / dt_s;
        }
        else
        {
            pid->derivative = 0.0f;
            pid->initialized = 1U;
        }
    }
    else
    {
        pid->derivative = 0.0f;
    }

    pid->output = pid->kp * pid->error
                + pid->ki * pid->integral
                + pid->kd * pid->derivative;
    pid->output = pid_clamp(pid->output,
                            pid->output_min,
                            pid->output_max);
    pid->previous_error = pid->error;
    return pid->output;
}

#ifndef __PID_H
#define __PID_H

#include "zf_common_headfile.h"

/*
 * Position PID relationship:
 *
 *   error      = target - feedback
 *   integral   = integral + error * dt
 *   derivative = (error - previous_error) / dt
 *   output     = kp * error + ki * integral + kd * derivative
 *
 * dt uses seconds. For a 10 ms control loop, pass 0.010f.
 * The integral state and final output are both limited to prevent windup and
 * to keep the result inside the actuator range.
 */
typedef struct
{
    float kp;
    float ki;
    float kd;

    float target;
    float feedback;
    float error;
    float previous_error;
    float integral;
    float derivative;
    float output;

    float integral_min;
    float integral_max;
    float output_min;
    float output_max;

    uint8 initialized;
} pid_t;

void pid_init(pid_t *pid,
              float kp, float ki, float kd,
              float output_min, float output_max,
              float integral_min, float integral_max);
void pid_reset(pid_t *pid);
void pid_set_gains(pid_t *pid, float kp, float ki, float kd);
void pid_set_limits(pid_t *pid,
                    float output_min, float output_max,
                    float integral_min, float integral_max);
float pid_calculate(pid_t *pid, float target, float feedback, float dt_s);

/*
 * Example:
 *
 *   static pid_t speed_pid;
 *
 *   pid_init(&speed_pid,
 *            1.0f, 0.1f, 0.01f,
 *            -10000.0f, 10000.0f,
 *            -5000.0f, 5000.0f);
 *
 *   // Called from a 10 ms module_Loop(), not from the 1 ms interrupt.
 *   motor_command = pid_calculate(&speed_pid,
 *                                 target_speed,
 *                                 measured_speed,
 *                                 0.010f);
 */

#endif

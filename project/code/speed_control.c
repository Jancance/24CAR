#include "speed_control.h"
#include "board_pins.h"

#include <stddef.h>

#define SPEED_CONTROL_PERIOD_MS       (20U)
#define SPEED_CONTROL_MAX_TARGET_MM_S (800.0f)
#define SPEED_CONTROL_MAX_SAFE_MM_S   (1200.0f)
#define SPEED_CONTROL_MAX_DUTY        (8000.0f)
#define SPEED_CONTROL_FILTER_ALPHA    (0.25f)
#define SPEED_CONTROL_MIN_ACCEL_MM_S2     (100.0f)
#define SPEED_CONTROL_MAX_ACCEL_MM_S2     (5000.0f)

static pid_t left_pid;
static pid_t right_pid;
static speed_control_state_t control_state;
static uint32 control_ms;
static uint8 left_reverse_count;
static uint8 right_reverse_count;
static float left_accel_mm_s2;
static float right_accel_mm_s2;

static float speed_control_clamp_gain(float value, float maximum)
{
    if (value < 0.0f)
    {
        return 0.0f;
    }
    return (value > maximum) ? maximum : value;
}

static float speed_control_clamp(float value, float minimum, float maximum)
{
    if (value < minimum)
    {
        return minimum;
    }
    if (value > maximum)
    {
        return maximum;
    }
    return value;
}

static float speed_control_integral_limit(float ki, float maximum_i_duty)
{
    if (ki > 0.0f)
    {
        return maximum_i_duty / ki;
    }
    return SPEED_CONTROL_MAX_DUTY;
}

static float speed_control_ramp(float value, float target, float maximum_step)
{
    if (value < target)
    {
        value += maximum_step;
        return (value > target) ? target : value;
    }
    if (value > target)
    {
        value -= maximum_step;
        return (value < target) ? target : value;
    }
    return value;
}

static void speed_control_stop_with_error(speed_control_error_t error)
{
    control_state.running = 0U;
    control_state.left_duty = 0;
    control_state.right_duty = 0;
    control_state.loop_dt_ms = 0U;
    control_state.left_target_mm_s = 0.0f;
    control_state.right_target_mm_s = 0.0f;
    control_state.left_setpoint_mm_s = 0.0f;
    control_state.right_setpoint_mm_s = 0.0f;
    control_state.error = error;
    motor_stop();
    pid_reset(&left_pid);
    pid_reset(&right_pid);
}

void speed_control_init(void)
{
    float left_integral_limit =
        speed_control_integral_limit(CAR_LEFT_SPEED_KI,
                                     CAR_LEFT_SPEED_I_MAX_DUTY);
    float right_integral_limit =
        speed_control_integral_limit(CAR_RIGHT_SPEED_KI,
                                     CAR_RIGHT_SPEED_I_MAX_DUTY);

    pid_init(&left_pid,
             CAR_LEFT_SPEED_KP, CAR_LEFT_SPEED_KI, CAR_LEFT_SPEED_KD,
             0.0f, SPEED_CONTROL_MAX_DUTY,
             -left_integral_limit, left_integral_limit);
    pid_init(&right_pid,
             CAR_RIGHT_SPEED_KP, CAR_RIGHT_SPEED_KI, CAR_RIGHT_SPEED_KD,
             0.0f, SPEED_CONTROL_MAX_DUTY,
             -right_integral_limit, right_integral_limit);
    control_state.left_target_mm_s = 0.0f;
    control_state.right_target_mm_s = 0.0f;
    control_state.left_setpoint_mm_s = 0.0f;
    control_state.right_setpoint_mm_s = 0.0f;
    control_state.left_raw_mm_s = 0.0f;
    control_state.right_raw_mm_s = 0.0f;
    control_state.left_speed_mm_s = 0.0f;
    control_state.right_speed_mm_s = 0.0f;
    control_state.left_setpoint_mm_s = 0.0f;
    control_state.right_setpoint_mm_s = 0.0f;
    control_state.left_duty = 0;
    control_state.right_duty = 0;
    control_state.running = 0U;
    control_state.ramp_enabled = 1U;
    control_state.error = SPEED_CONTROL_ERROR_NONE;
    left_reverse_count = 0U;
    right_reverse_count = 0U;
    left_accel_mm_s2 = CAR_LEFT_SPEED_ACCEL_MM_S2;
    right_accel_mm_s2 = CAR_RIGHT_SPEED_ACCEL_MM_S2;
    control_ms = system_get_ms();
    motor_stop();
}

void speed_control_start(void)
{
    car_encoder_get_delta(NULL, NULL);
    pid_reset(&left_pid);
    pid_reset(&right_pid);
    control_state.left_raw_mm_s = 0.0f;
    control_state.right_raw_mm_s = 0.0f;
    control_state.left_speed_mm_s = 0.0f;
    control_state.right_speed_mm_s = 0.0f;
    control_state.error = SPEED_CONTROL_ERROR_NONE;
    left_reverse_count = 0U;
    right_reverse_count = 0U;
    control_ms = system_get_ms();
    control_state.running = 1U;
}

void speed_control_stop(void)
{
    speed_control_stop_with_error(SPEED_CONTROL_ERROR_NONE);
}

void speed_control_set_target(float left_mm_s, float right_mm_s)
{
    control_state.left_target_mm_s =
        speed_control_clamp(left_mm_s, 0.0f, SPEED_CONTROL_MAX_TARGET_MM_S);
    control_state.right_target_mm_s =
        speed_control_clamp(right_mm_s, 0.0f, SPEED_CONTROL_MAX_TARGET_MM_S);
}

void speed_control_set_target_trim(float base_mm_s, float trim_mm_s)
{
    speed_control_set_target(base_mm_s - trim_mm_s,
                             base_mm_s + trim_mm_s);
}

void speed_control_set_acceleration(float left_mm_s2, float right_mm_s2)
{
    left_accel_mm_s2 = speed_control_clamp(left_mm_s2,
                                           SPEED_CONTROL_MIN_ACCEL_MM_S2,
                                           SPEED_CONTROL_MAX_ACCEL_MM_S2);
    right_accel_mm_s2 = speed_control_clamp(right_mm_s2,
                                            SPEED_CONTROL_MIN_ACCEL_MM_S2,
                                            SPEED_CONTROL_MAX_ACCEL_MM_S2);
}

void speed_control_set_ramp_enabled(uint8 enabled)
{
    control_state.ramp_enabled = enabled ? 1U : 0U;
}

void speed_control_set_left_gains(float kp, float ki, float kd)
{
    float integral_limit;

    kp = speed_control_clamp_gain(kp, 100.0f);
    ki = speed_control_clamp_gain(ki, 300.0f);
    kd = speed_control_clamp_gain(kd, 20.0f);
    pid_set_gains(&left_pid, kp, ki, kd);
    integral_limit = speed_control_integral_limit(
        ki, CAR_LEFT_SPEED_I_MAX_DUTY);
    pid_set_limits(&left_pid, 0.0f, SPEED_CONTROL_MAX_DUTY,
                   -integral_limit, integral_limit);
}

void speed_control_set_right_gains(float kp, float ki, float kd)
{
    float integral_limit;

    kp = speed_control_clamp_gain(kp, 100.0f);
    ki = speed_control_clamp_gain(ki, 300.0f);
    kd = speed_control_clamp_gain(kd, 20.0f);
    pid_set_gains(&right_pid, kp, ki, kd);
    integral_limit = speed_control_integral_limit(
        ki, CAR_RIGHT_SPEED_I_MAX_DUTY);
    pid_set_limits(&right_pid, 0.0f, SPEED_CONTROL_MAX_DUTY,
                   -integral_limit, integral_limit);
}

void speed_control_get_gains(float *left_kp, float *left_ki, float *left_kd,
                             float *right_kp, float *right_ki, float *right_kd)
{
    if (left_kp != NULL)  { *left_kp = left_pid.kp; }
    if (left_ki != NULL)  { *left_ki = left_pid.ki; }
    if (left_kd != NULL)  { *left_kd = left_pid.kd; }
    if (right_kp != NULL) { *right_kp = right_pid.kp; }
    if (right_ki != NULL) { *right_ki = right_pid.ki; }
    if (right_kd != NULL) { *right_kd = right_pid.kd; }
}

void speed_control_get_state(speed_control_state_t *state)
{
    if (state != NULL)
    {
        *state = control_state;
    }
}

void speed_control_loop(void)
{
    uint32 now = system_get_ms();
    uint32 elapsed_ms;
    int32 left_delta;
    int32 right_delta;
    float left_output;
    float right_output;
    float dt_s;
    float left_setpoint_step;
    float right_setpoint_step;

    elapsed_ms = (uint32)(now - control_ms);
    if (elapsed_ms < SPEED_CONTROL_PERIOD_MS)
    {
        return;
    }
    control_ms = now;
    control_state.loop_dt_ms = elapsed_ms;
    dt_s = (float)elapsed_ms / 1000.0f;
    left_setpoint_step = left_accel_mm_s2 * dt_s;
    right_setpoint_step = right_accel_mm_s2 * dt_s;

    car_encoder_get_delta(&left_delta, &right_delta);
    control_state.left_raw_mm_s = car_encoder_count_to_mm(left_delta)
                                      * (1000.0f / (float)elapsed_ms);
    control_state.right_raw_mm_s = car_encoder_count_to_mm(right_delta)
                                       * (1000.0f / (float)elapsed_ms);
    control_state.left_speed_mm_s += SPEED_CONTROL_FILTER_ALPHA
        * (control_state.left_raw_mm_s - control_state.left_speed_mm_s);
    control_state.right_speed_mm_s += SPEED_CONTROL_FILTER_ALPHA
        * (control_state.right_raw_mm_s - control_state.right_speed_mm_s);

    if (!control_state.running)
    {
        motor_stop();
        return;
    }

    if (control_state.ramp_enabled)
    {
        control_state.left_setpoint_mm_s =
            speed_control_ramp(control_state.left_setpoint_mm_s,
                               control_state.left_target_mm_s,
                               left_setpoint_step);
        control_state.right_setpoint_mm_s =
            speed_control_ramp(control_state.right_setpoint_mm_s,
                               control_state.right_target_mm_s,
                               right_setpoint_step);
    }
    else
    {
        control_state.left_setpoint_mm_s = control_state.left_target_mm_s;
        control_state.right_setpoint_mm_s = control_state.right_target_mm_s;
    }

    left_reverse_count = (control_state.left_raw_mm_s < -20.0f)
                       ? (uint8)(left_reverse_count + 1U) : 0U;
    right_reverse_count = (control_state.right_raw_mm_s < -20.0f)
                        ? (uint8)(right_reverse_count + 1U) : 0U;
    if ((left_reverse_count >= 3U) || (right_reverse_count >= 3U))
    {
        speed_control_stop_with_error(SPEED_CONTROL_ERROR_ENCODER_REVERSED);
        return;
    }

    if ((control_state.left_speed_mm_s > SPEED_CONTROL_MAX_SAFE_MM_S) ||
        (control_state.left_speed_mm_s < -SPEED_CONTROL_MAX_SAFE_MM_S) ||
        (control_state.right_speed_mm_s > SPEED_CONTROL_MAX_SAFE_MM_S) ||
        (control_state.right_speed_mm_s < -SPEED_CONTROL_MAX_SAFE_MM_S))
    {
        speed_control_stop_with_error(SPEED_CONTROL_ERROR_OVERSPEED);
        return;
    }

    left_output = pid_calculate(&left_pid,
                                control_state.left_setpoint_mm_s,
                                control_state.left_speed_mm_s,
                                dt_s);
    right_output = pid_calculate(&right_pid,
                                 control_state.right_setpoint_mm_s,
                                 control_state.right_speed_mm_s,
                                 dt_s);
    control_state.left_duty = (int16)left_output;
    control_state.right_duty = (int16)right_output;
    motor_set(control_state.left_duty, control_state.right_duty);
}

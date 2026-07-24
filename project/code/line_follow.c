#include "line_follow.h"
#include "board_pins.h"

#include <stddef.h>

#define LINE_FOLLOW_PERIOD_MS       (40U)
#define LINE_FOLLOW_LOST_HOLD_MS    (150U)
#define LINE_FOLLOW_MIN_SPEED_MM_S  (100.0f)
#define LINE_FOLLOW_MAX_SPEED_MM_S  (400.0f)
#define LINE_FOLLOW_MIN_WHEEL_TARGET_MM_S (100.0f)
#define LINE_FOLLOW_MAX_TRIM_MM_S   (50.0f)
#define LINE_FOLLOW_SEARCH_TRIM_MM_S (30.0f)
#define LINE_FOLLOW_DEFAULT_KP       (10.0f)
#define LINE_FOLLOW_DEFAULT_KI       (0.0f)
#define LINE_FOLLOW_DEFAULT_KD       (0.0f)
#define LINE_FOLLOW_ERROR_DEADBAND    (0.5f)
#define LINE_FOLLOW_GAIN_MAX         (20.0f)
#define LINE_FOLLOW_I_TRIM_LIMIT     (20.0f)
#define LINE_FOLLOW_DERIVATIVE_LIMIT (8.0f)
#define LINE_FOLLOW_DERIVATIVE_ALPHA (0.25f)
#define LINE_FOLLOW_WEIGHT_SCALE    (10.0f)

static const int8 line_weights[8] = {-35, -25, -15, -5, 5, 15, 25, 35};
static line_follow_state_t line_state;
static uint32 line_control_ms;
static uint32 line_last_seen_ms;

static float line_follow_clamp(float value, float minimum, float maximum)
{
    if (value < minimum) {
        return minimum;
    }
    if (value > maximum) {
        return maximum;
    }
    return value;
}

static uint8 line_follow_sample(float *error, uint8 *mask)
{
    uint8 index;
    uint8 active_count = 0U;
    uint8 active_mask = 0U;
    int32 weighted_sum = 0;

    gray_update();
    for (index = 0U; index < CAR_GRAY_COUNT; index++)
    {
        if (gray_values[index])
        {
            active_count++;
            active_mask |= (uint8)(1U << index);
            weighted_sum += line_weights[index];
        }
    }
    *mask = active_mask;
    if (active_count == 0U)
    {
        return 0U;
    }
    *error = ((float)weighted_sum / (float)active_count) /
             LINE_FOLLOW_WEIGHT_SCALE;
    return 1U;
}

uint8 line_follow_sample_now(float *error, uint8 *mask)
{
    if ((error == NULL) || (mask == NULL))
    {
        return 0U;
    }
    return line_follow_sample(error, mask);
}

void line_follow_init(void)
{
    line_state.base_speed_mm_s = 0.0f;
    line_state.commanded_speed_mm_s = 0.0f;
    line_state.error = 0.0f;
    line_state.integral = 0.0f;
    line_state.derivative = 0.0f;
    line_state.trim_mm_s = 0.0f;
    line_state.kp = LINE_FOLLOW_DEFAULT_KP;
    line_state.ki = LINE_FOLLOW_DEFAULT_KI;
    line_state.kd = LINE_FOLLOW_DEFAULT_KD;
    line_state.active = 0U;
    line_state.line_detected = 0U;
    line_state.active_mask = 0U;
    line_control_ms = system_get_ms();
    line_last_seen_ms = line_control_ms;
}

uint8 line_follow_start(float base_speed_mm_s)
{
    float initial_error;
    uint8 initial_mask;

    if (!line_follow_sample(&initial_error, &initial_mask))
    {
        line_state.line_detected = 0U;
        line_state.active_mask = initial_mask;
        return 0U;
    }
    if (base_speed_mm_s < LINE_FOLLOW_MIN_SPEED_MM_S)
    {
        base_speed_mm_s = LINE_FOLLOW_MIN_SPEED_MM_S;
    }
    if (base_speed_mm_s > LINE_FOLLOW_MAX_SPEED_MM_S)
    {
        base_speed_mm_s = LINE_FOLLOW_MAX_SPEED_MM_S;
    }
    position_control_stop();
    line_state.base_speed_mm_s = base_speed_mm_s;
    line_state.commanded_speed_mm_s = base_speed_mm_s;
    line_state.error = initial_error;
    line_state.integral = 0.0f;
    line_state.derivative = 0.0f;
    line_state.trim_mm_s = 0.0f;
    line_state.active = 1U;
    line_state.line_detected = 1U;
    line_state.active_mask = initial_mask;
    line_control_ms = system_get_ms();
    line_last_seen_ms = line_control_ms;
    speed_control_set_ramp_enabled(1U);
    speed_control_set_target_trim(base_speed_mm_s, 0.0f);
    speed_control_start();
    return 1U;
}

void line_follow_stop(void)
{
    line_state.active = 0U;
    line_state.integral = 0.0f;
    line_state.trim_mm_s = 0.0f;
    speed_control_stop();
}

void line_follow_set_gains(float kp, float ki, float kd)
{
    line_state.kp = line_follow_clamp(kp, 0.0f, LINE_FOLLOW_GAIN_MAX);
    line_state.ki = line_follow_clamp(ki, 0.0f, LINE_FOLLOW_GAIN_MAX);
    line_state.kd = line_follow_clamp(kd, 0.0f, LINE_FOLLOW_GAIN_MAX);
    if (line_state.ki > 0.0f)
    {
        line_state.integral = line_follow_clamp(
            line_state.integral,
            -LINE_FOLLOW_I_TRIM_LIMIT / line_state.ki,
            LINE_FOLLOW_I_TRIM_LIMIT / line_state.ki);
    }
    else
    {
        line_state.integral = 0.0f;
    }
}

uint8 line_follow_is_active(void)
{
    return line_state.active;
}

void line_follow_get_state(line_follow_state_t *state)
{
    if (state != NULL)
    {
        *state = line_state;
    }
}

void line_follow_loop(void)
{
    uint32 now_ms;
    uint32 elapsed_ms;
    float error;
    float derivative;
    float trim;
    float speed;
    float absolute_trim;
    uint8 mask;
    uint8 detected;

    now_ms = system_get_ms();
    if ((uint32)(now_ms - line_control_ms) < LINE_FOLLOW_PERIOD_MS)
    {
        return;
    }
    elapsed_ms = (uint32)(now_ms - line_control_ms);
    line_control_ms = now_ms;
    if (!line_state.active)
    {
        return;
    }

    detected = line_follow_sample(&error, &mask);
    line_state.active_mask = mask;
    line_state.line_detected = detected;
    if (detected)
    {
        line_last_seen_ms = now_ms;
        if ((error >= -LINE_FOLLOW_ERROR_DEADBAND) &&
            (error <= LINE_FOLLOW_ERROR_DEADBAND))
        {
            error = 0.0f;
        }
        derivative = (error - line_state.error) /
                     ((float)elapsed_ms / 1000.0f);
        derivative = line_follow_clamp(derivative,
                                       -LINE_FOLLOW_DERIVATIVE_LIMIT,
                                       LINE_FOLLOW_DERIVATIVE_LIMIT);
        derivative = line_state.derivative + LINE_FOLLOW_DERIVATIVE_ALPHA *
                     (derivative - line_state.derivative);
        line_state.error = error;
        if (line_state.ki > 0.0f)
        {
            line_state.integral += error * ((float)elapsed_ms / 1000.0f);
            line_state.integral = line_follow_clamp(
                line_state.integral,
                -LINE_FOLLOW_I_TRIM_LIMIT / line_state.ki,
                LINE_FOLLOW_I_TRIM_LIMIT / line_state.ki);
        }
        else
        {
            line_state.integral = 0.0f;
        }
        line_state.derivative = derivative;
        speed = line_state.base_speed_mm_s -
                (float)(error < 0.0f ? -error : error) * 4.0f;
        trim = -(line_state.kp * error +
                 line_state.ki * line_state.integral +
                 line_state.kd * derivative);
    }
    else
    {
        line_state.derivative = 0.0f;
        speed = LINE_FOLLOW_MIN_SPEED_MM_S;
        if ((uint32)(now_ms - line_last_seen_ms) <= LINE_FOLLOW_LOST_HOLD_MS)
        {
            trim = -line_state.kp * line_state.error;
        }
        else
        {
            trim = (line_state.error >= 0.0f) ?
                   -LINE_FOLLOW_SEARCH_TRIM_MM_S : LINE_FOLLOW_SEARCH_TRIM_MM_S;
        }
    }
    line_state.trim_mm_s = line_follow_clamp(
        trim, -LINE_FOLLOW_MAX_TRIM_MM_S, LINE_FOLLOW_MAX_TRIM_MM_S);
    absolute_trim = (line_state.trim_mm_s < 0.0f) ?
                    -line_state.trim_mm_s : line_state.trim_mm_s;
    speed = line_follow_clamp(speed,
                              LINE_FOLLOW_MIN_SPEED_MM_S,
                              LINE_FOLLOW_MAX_SPEED_MM_S);
    if (speed < LINE_FOLLOW_MIN_WHEEL_TARGET_MM_S + absolute_trim)
    {
        speed = LINE_FOLLOW_MIN_WHEEL_TARGET_MM_S + absolute_trim;
    }
    line_state.commanded_speed_mm_s = speed;
    speed_control_set_target_trim(line_state.commanded_speed_mm_s,
                                  line_state.trim_mm_s);
}

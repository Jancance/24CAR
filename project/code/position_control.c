#include "position_control.h"
#include "board_pins.h"

#include <stddef.h>

#define POSITION_CONTROL_PERIOD_MS          (20U)
#define POSITION_CONTROL_MAX_TRIM_MM_S      (30.0f)
#define POSITION_CONTROL_STARTUP_TRIM_MM_S  (10.0f)
#define POSITION_CONTROL_STARTUP_HOLD_MS    (500U)
#define POSITION_CONTROL_STARTUP_SPEED_MM_S (120.0f)

static position_control_state_t control_state;
static uint32 control_ms;
static uint32 startup_ms;

static float position_control_clamp(float value, float minimum, float maximum)
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

static float position_control_wrap_angle(float angle_deg)
{
    while (angle_deg > 180.0f)
    {
        angle_deg -= 360.0f;
    }
    while (angle_deg < -180.0f)
    {
        angle_deg += 360.0f;
    }
    return angle_deg;
}

static float position_control_startup_speed(uint32 now_ms,
                                            float target_speed_mm_s)
{
    if ((uint32)(now_ms - startup_ms) < POSITION_CONTROL_STARTUP_HOLD_MS)
    {
        return (target_speed_mm_s < POSITION_CONTROL_STARTUP_SPEED_MM_S) ?
               target_speed_mm_s : POSITION_CONTROL_STARTUP_SPEED_MM_S;
    }
    return target_speed_mm_s;
}

static float position_control_trim_limit(uint32 now_ms)
{
    if ((uint32)(now_ms - startup_ms) < POSITION_CONTROL_STARTUP_HOLD_MS)
    {
        return POSITION_CONTROL_STARTUP_TRIM_MM_S;
    }
    return POSITION_CONTROL_MAX_TRIM_MM_S;
}

void position_control_init(void)
{
    control_state.base_speed_mm_s = 0.0f;
    control_state.target_yaw_deg = 0.0f;
    control_state.yaw_deg = 0.0f;
    control_state.yaw_rate_dps = 0.0f;
    control_state.yaw_error_deg = 0.0f;
    control_state.trim_mm_s = 0.0f;
    control_state.kp = CAR_YAW_POSITION_KP;
    control_state.kd = CAR_YAW_POSITION_KD;
    control_state.active = 0U;
    control_state.icm_calibrated = 0U;
    control_state.error = POSITION_CONTROL_ERROR_NONE;
    control_ms = system_get_ms();
    startup_ms = control_ms;
}

uint8 position_control_start(float base_speed_mm_s)
{
    icm45686_attitude_t attitude;

    icm45686_get_attitude(&attitude);
    if (!attitude.calibrated)
    {
        control_state.error = POSITION_CONTROL_ERROR_ICM_NOT_READY;
        return 0U;
    }
    icm45686_reset_yaw();
    control_state.base_speed_mm_s = base_speed_mm_s;
    control_state.target_yaw_deg = 0.0f;
    control_state.yaw_error_deg = 0.0f;
    control_state.trim_mm_s = 0.0f;
    control_state.error = POSITION_CONTROL_ERROR_NONE;
    control_state.active = 1U;
    control_ms = system_get_ms();
    startup_ms = control_ms;
    speed_control_set_ramp_enabled(1U);
    speed_control_set_target_trim(position_control_startup_speed(
                                      control_ms, base_speed_mm_s),
                                  0.0f);
    speed_control_start();
    return 1U;
}

void position_control_stop(void)
{
    control_state.active = 0U;
    control_state.trim_mm_s = 0.0f;
    speed_control_stop();
}

void position_control_get_state(position_control_state_t *state)
{
    if (state != NULL)
    {
        *state = control_state;
    }
}

void position_control_loop(void)
{
    icm45686_attitude_t attitude;
    uint32 now_ms;
    uint32 elapsed_ms;
    float trim;

    (void)icm45686_update();
    icm45686_get_attitude(&attitude);
    control_state.icm_calibrated = attitude.calibrated;
    control_state.yaw_deg = attitude.yaw;
    control_state.yaw_rate_dps = attitude.yaw_rate_dps;
    now_ms = system_get_ms();

    if (!control_state.active)
    {
        return;
    }
    if (!attitude.calibrated)
    {
        control_state.error = POSITION_CONTROL_ERROR_ICM_NOT_READY;
        position_control_stop();
        return;
    }

    elapsed_ms = (uint32)(now_ms - control_ms);
    if (elapsed_ms < POSITION_CONTROL_PERIOD_MS)
    {
        return;
    }
    control_ms = now_ms;
    control_state.yaw_error_deg = position_control_wrap_angle(
        control_state.target_yaw_deg - control_state.yaw_deg);
    trim = control_state.kp * control_state.yaw_error_deg
         - control_state.kd * control_state.yaw_rate_dps;
    trim *= CAR_YAW_POSITION_TRIM_SIGN;
    {
        float trim_limit = position_control_trim_limit(now_ms);
        control_state.trim_mm_s = position_control_clamp(
            trim, -trim_limit, trim_limit);
    }
    speed_control_set_target_trim(position_control_startup_speed(
                                      now_ms, control_state.base_speed_mm_s),
                                  control_state.trim_mm_s);
}

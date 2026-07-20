#include "gimbal.h"

static float gimbal_target_yaw_deg;
static float gimbal_target_pitch_deg;

static uint8 gimbal_in_range(float value, float minimum, float maximum)
{
    return ((value >= minimum) && (value <= maximum)) ? 1U : 0U;
}

static int32 gimbal_deg_to_pulse(float degree, float reduction_ratio, uint8 reversed)
{
    float pulse = degree * GIMBAL_MOTOR_PULSES_PER_REV * reduction_ratio / 360.0f;
    if (reversed)
    {
        pulse = -pulse;
    }
    return (int32)((pulse >= 0.0f) ? (pulse + 0.5f) : (pulse - 0.5f));
}

static float gimbal_count_to_deg(int32 count, float reduction_ratio, uint8 reversed)
{
    float degree = (float)count * 360.0f
                 / (ZDT_POSITION_FEEDBACK_PER_REV * reduction_ratio);
    return reversed ? -degree : degree;
}

void gimbal_init(void)
{
    gimbal_target_yaw_deg = 0.0f;
    gimbal_target_pitch_deg = 0.0f;
    zdt_init(ZDT_PORT_YAW);
    zdt_init(ZDT_PORT_PITCH);
}

void gimbal_enable(void)
{
    zdt_enable(ZDT_PORT_YAW, GIMBAL_YAW_ADDR, 0U);
    zdt_enable(ZDT_PORT_PITCH, GIMBAL_PITCH_ADDR, 0U);
}

void gimbal_disable(void)
{
    zdt_disable(ZDT_PORT_YAW, GIMBAL_YAW_ADDR, 0U);
    zdt_disable(ZDT_PORT_PITCH, GIMBAL_PITCH_ADDR, 0U);
}

void gimbal_set_zero(void)
{
    zdt_set_current_zero(ZDT_PORT_YAW, GIMBAL_YAW_ADDR);
    zdt_set_current_zero(ZDT_PORT_PITCH, GIMBAL_PITCH_ADDR);
    gimbal_target_yaw_deg = 0.0f;
    gimbal_target_pitch_deg = 0.0f;
}

uint8 gimbal_set_angle(float yaw_deg, float pitch_deg)
{
    int32 yaw_pulse;
    int32 pitch_pulse;

    if (!gimbal_in_range(yaw_deg, GIMBAL_YAW_MIN_DEG, GIMBAL_YAW_MAX_DEG)
        || !gimbal_in_range(pitch_deg, GIMBAL_PITCH_MIN_DEG, GIMBAL_PITCH_MAX_DEG))
    {
        return 0U;
    }

    yaw_pulse = gimbal_deg_to_pulse(yaw_deg,
                                    GIMBAL_YAW_REDUCTION_RATIO,
                                    GIMBAL_YAW_REVERSED);
    pitch_pulse = gimbal_deg_to_pulse(pitch_deg,
                                      GIMBAL_PITCH_REDUCTION_RATIO,
                                      GIMBAL_PITCH_REVERSED);

    // Load both targets first, then trigger each independent UART link.
    zdt_move_absolute(ZDT_PORT_YAW, GIMBAL_YAW_ADDR, yaw_pulse,
                      GIMBAL_DEFAULT_RPM, GIMBAL_DEFAULT_ACCELERATION, 1U);
    zdt_move_absolute(ZDT_PORT_PITCH, GIMBAL_PITCH_ADDR, pitch_pulse,
                      GIMBAL_DEFAULT_RPM, GIMBAL_DEFAULT_ACCELERATION, 1U);
    zdt_synchronous_start(ZDT_PORT_YAW, GIMBAL_YAW_ADDR);
    zdt_synchronous_start(ZDT_PORT_PITCH, GIMBAL_PITCH_ADDR);

    gimbal_target_yaw_deg = yaw_deg;
    gimbal_target_pitch_deg = pitch_deg;
    return 1U;
}

uint8 gimbal_set_yaw(float yaw_deg)
{
    int32 pulse;
    if (!gimbal_in_range(yaw_deg, GIMBAL_YAW_MIN_DEG, GIMBAL_YAW_MAX_DEG))
    {
        return 0U;
    }
    pulse = gimbal_deg_to_pulse(yaw_deg,
                                GIMBAL_YAW_REDUCTION_RATIO,
                                GIMBAL_YAW_REVERSED);
    zdt_move_absolute(ZDT_PORT_YAW, GIMBAL_YAW_ADDR, pulse,
                      GIMBAL_DEFAULT_RPM, GIMBAL_DEFAULT_ACCELERATION, 0U);
    gimbal_target_yaw_deg = yaw_deg;
    return 1U;
}

uint8 gimbal_set_pitch(float pitch_deg)
{
    int32 pulse;
    if (!gimbal_in_range(pitch_deg, GIMBAL_PITCH_MIN_DEG, GIMBAL_PITCH_MAX_DEG))
    {
        return 0U;
    }
    pulse = gimbal_deg_to_pulse(pitch_deg,
                                GIMBAL_PITCH_REDUCTION_RATIO,
                                GIMBAL_PITCH_REVERSED);
    zdt_move_absolute(ZDT_PORT_PITCH, GIMBAL_PITCH_ADDR, pulse,
                      GIMBAL_DEFAULT_RPM, GIMBAL_DEFAULT_ACCELERATION, 0U);
    gimbal_target_pitch_deg = pitch_deg;
    return 1U;
}

uint8 gimbal_move_relative(float yaw_delta_deg, float pitch_delta_deg)
{
    return gimbal_set_angle(gimbal_target_yaw_deg + yaw_delta_deg,
                            gimbal_target_pitch_deg + pitch_delta_deg);
}

void gimbal_request_angle(void)
{
    zdt_read_position(ZDT_PORT_YAW, GIMBAL_YAW_ADDR);
    zdt_read_position(ZDT_PORT_PITCH, GIMBAL_PITCH_ADDR);
}

void gimbal_request_status(void)
{
    zdt_read_status(ZDT_PORT_YAW, GIMBAL_YAW_ADDR);
    zdt_read_status(ZDT_PORT_PITCH, GIMBAL_PITCH_ADDR);
}

void gimbal_update(void)
{
    zdt_update(ZDT_PORT_YAW);
    zdt_update(ZDT_PORT_PITCH);
}

void gimbal_get_angle_state(gimbal_angle_t *angle)
{
    zdt_state_t yaw_state;
    zdt_state_t pitch_state;

    if (angle == NULL)
    {
        return;
    }

    zdt_get_state(ZDT_PORT_YAW, &yaw_state);
    zdt_get_state(ZDT_PORT_PITCH, &pitch_state);
    angle->yaw_deg = gimbal_count_to_deg(yaw_state.position_count,
                                         GIMBAL_YAW_REDUCTION_RATIO,
                                         GIMBAL_YAW_REVERSED);
    angle->pitch_deg = gimbal_count_to_deg(pitch_state.position_count,
                                           GIMBAL_PITCH_REDUCTION_RATIO,
                                           GIMBAL_PITCH_REVERSED);
    angle->yaw_valid = yaw_state.position_valid;
    angle->pitch_valid = pitch_state.position_valid;
}

uint8 gimbal_get_angle(float *yaw_deg, float *pitch_deg)
{
    gimbal_angle_t angle;
    if ((yaw_deg == NULL) || (pitch_deg == NULL))
    {
        return 0U;
    }
    gimbal_get_angle_state(&angle);
    *yaw_deg = angle.yaw_deg;
    *pitch_deg = angle.pitch_deg;
    return (angle.yaw_valid && angle.pitch_valid) ? 1U : 0U;
}

void gimbal_stop(void)
{
    zdt_stop(ZDT_PORT_YAW, GIMBAL_YAW_ADDR, 1U);
    zdt_stop(ZDT_PORT_PITCH, GIMBAL_PITCH_ADDR, 1U);
    zdt_synchronous_start(ZDT_PORT_YAW, GIMBAL_YAW_ADDR);
    zdt_synchronous_start(ZDT_PORT_PITCH, GIMBAL_PITCH_ADDR);
}

void gimbal_emergency_stop(void)
{
    // Keep both axes enabled after stopping so the pitch axis cannot fall under gravity.
    zdt_stop(ZDT_PORT_YAW, GIMBAL_YAW_ADDR, 0U);
    zdt_stop(ZDT_PORT_PITCH, GIMBAL_PITCH_ADDR, 0U);
}

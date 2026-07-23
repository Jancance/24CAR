#ifndef __SPEED_CONTROL_H
#define __SPEED_CONTROL_H

#include "zf_common_headfile.h"

typedef enum
{
    SPEED_CONTROL_ERROR_NONE = 0,
    SPEED_CONTROL_ERROR_ENCODER_REVERSED,
    SPEED_CONTROL_ERROR_OVERSPEED
} speed_control_error_t;

typedef struct
{
    float left_target_mm_s;
    float right_target_mm_s;
    float left_setpoint_mm_s;
    float right_setpoint_mm_s;
    float left_raw_mm_s;
    float right_raw_mm_s;
    float left_speed_mm_s;
    float right_speed_mm_s;
    int16 left_duty;
    int16 right_duty;
    uint8 running;
    speed_control_error_t error;
} speed_control_state_t;

void speed_control_init(void);
void speed_control_start(void);
void speed_control_stop(void);
void speed_control_set_target(float left_mm_s, float right_mm_s);
/* Positive trim slows the left wheel and speeds up the right wheel. */
void speed_control_set_target_trim(float base_mm_s, float trim_mm_s);
void speed_control_set_acceleration(float left_mm_s2, float right_mm_s2);
void speed_control_set_left_gains(float kp, float ki, float kd);
void speed_control_set_right_gains(float kp, float ki, float kd);
void speed_control_get_gains(float *left_kp, float *left_ki, float *left_kd,
                             float *right_kp, float *right_ki, float *right_kd);
void speed_control_get_state(speed_control_state_t *state);
void speed_control_loop(void);

#endif

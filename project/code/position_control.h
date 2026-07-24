#ifndef __POSITION_CONTROL_H
#define __POSITION_CONTROL_H

#include "zf_common_headfile.h"

typedef enum
{
    POSITION_CONTROL_ERROR_NONE = 0,
    POSITION_CONTROL_ERROR_ICM_NOT_READY
} position_control_error_t;

typedef struct
{
    float base_speed_mm_s;
    float target_yaw_deg;
    float yaw_deg;
    float yaw_rate_dps;
    float yaw_error_deg;
    float trim_mm_s;
    float kp;
    float kd;
    uint8 active;
    uint8 icm_calibrated;
    position_control_error_t error;
} position_control_state_t;

void position_control_init(void);
void position_control_loop(void);
uint8 position_control_start(float base_speed_mm_s);
void position_control_stop(void);
void position_control_get_state(position_control_state_t *state);

#endif

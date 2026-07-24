#ifndef __LINE_FOLLOW_H
#define __LINE_FOLLOW_H

#include "zf_common_headfile.h"

typedef struct
{
    float base_speed_mm_s;
    float commanded_speed_mm_s;
    float error;
    float integral;
    float derivative;
    float trim_mm_s;
    float kp;
    float ki;
    float kd;
    uint8 active;
    uint8 line_detected;
    uint8 active_mask;
} line_follow_state_t;

void line_follow_init(void);
void line_follow_loop(void);
uint8 line_follow_sample_now(float *error, uint8 *mask);
uint8 line_follow_start(float base_speed_mm_s);
void line_follow_stop(void);
void line_follow_set_gains(float kp, float ki, float kd);
uint8 line_follow_is_active(void);
void line_follow_get_state(line_follow_state_t *state);

#endif

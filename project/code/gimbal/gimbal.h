#ifndef __GIMBAL_H
#define __GIMBAL_H

#include "zdt.h"

// Mechanical configuration. Verify these values on the real two-axis gimbal.
#define GIMBAL_YAW_ADDR              (ZDT_DEFAULT_ADDR)
#define GIMBAL_PITCH_ADDR            (ZDT_DEFAULT_ADDR)
#define GIMBAL_YAW_MIN_DEG           (-180.0f)
#define GIMBAL_YAW_MAX_DEG           (180.0f)
#define GIMBAL_PITCH_MIN_DEG         (-90.0f)
#define GIMBAL_PITCH_MAX_DEG         (90.0f)
#define GIMBAL_YAW_REVERSED          (0U)
#define GIMBAL_PITCH_REVERSED        (1U)

// Official example: 16 microsteps gives 3200 command pulses per motor revolution.
// Change this if the X42S subdivision or the mechanical reduction ratio changes.
#define GIMBAL_MOTOR_PULSES_PER_REV  (3200.0f)
#define GIMBAL_YAW_REDUCTION_RATIO   (1.0f)
#define GIMBAL_PITCH_REDUCTION_RATIO (1.0f)
#define GIMBAL_DEFAULT_RPM           (60U)
#define GIMBAL_DEFAULT_ACCELERATION  (20U)

typedef struct
{
    float yaw_deg;
    float pitch_deg;
    uint8 yaw_valid;
    uint8 pitch_valid;
} gimbal_angle_t;

void gimbal_init(void);
void gimbal_enable(void);
void gimbal_disable(void);
void gimbal_set_zero(void);

// Return 1 when the requested angle is inside both software limits; otherwise send nothing.
uint8 gimbal_set_angle(float yaw_deg, float pitch_deg);
uint8 gimbal_set_yaw(float yaw_deg);
uint8 gimbal_set_pitch(float pitch_deg);
uint8 gimbal_move_relative(float yaw_delta_deg, float pitch_delta_deg);

// Non-blocking feedback flow: request -> repeatedly update -> get cached angle.
void gimbal_request_angle(void);
void gimbal_request_status(void);
void gimbal_update(void);
uint8 gimbal_get_angle(float *yaw_deg, float *pitch_deg);
void gimbal_get_angle_state(gimbal_angle_t *angle);

void gimbal_stop(void);
void gimbal_emergency_stop(void);

#endif

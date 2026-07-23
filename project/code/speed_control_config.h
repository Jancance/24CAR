#ifndef __SPEED_CONTROL_CONFIG_H
#define __SPEED_CONTROL_CONFIG_H

/* Bench-tuned left wheel values. */
#define CAR_LEFT_SPEED_KP (20.0f)
#define CAR_LEFT_SPEED_KI (80.0f)
#define CAR_LEFT_SPEED_KD (0.0f)
#define CAR_LEFT_SPEED_I_MAX_DUTY (4000.0f)
#define CAR_LEFT_SPEED_ACCEL_MM_S2 (300.0f)

/* Right wheel bench-tuned at 250/400/600 mm/s with load disturbance. */
#define CAR_RIGHT_SPEED_KP (20.0f)
#define CAR_RIGHT_SPEED_KI (50.0f)
#define CAR_RIGHT_SPEED_KD (0.0f)
#define CAR_RIGHT_SPEED_I_MAX_DUTY (4500.0f)
#define CAR_RIGHT_SPEED_ACCEL_MM_S2 (300.0f)

/* Verified static straight-line baseline from the 10 s ground tests. */
#define CAR_STRAIGHT_LEFT_BASE_MM_S  (248.5f)
#define CAR_STRAIGHT_RIGHT_BASE_MM_S (251.5f)

/* Yaw outer loop: trim = Kp * yaw_error - Kd * calibrated yaw rate. */
#define CAR_YAW_POSITION_KP       (3.3f)
#define CAR_YAW_POSITION_KD       (1.18f)
#define CAR_YAW_POSITION_TRIM_SIGN (1.0f)
#define CAR_YAW_POSITION_START_SPEED_MM_S (100.0f)

#endif

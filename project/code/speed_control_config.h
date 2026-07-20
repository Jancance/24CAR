#ifndef __SPEED_CONTROL_CONFIG_H
#define __SPEED_CONTROL_CONFIG_H

/* Bench-tuned left wheel values. */
#define CAR_LEFT_SPEED_KP (20.0f)
#define CAR_LEFT_SPEED_KI (80.0f)
#define CAR_LEFT_SPEED_KD (0.0f)
#define CAR_LEFT_SPEED_I_MAX_DUTY (4000.0f)
#define CAR_LEFT_SPEED_ACCEL_MM_S2 (500.0f)

/* Right wheel bench-tuned at 250/400/600 mm/s with load disturbance. */
#define CAR_RIGHT_SPEED_KP (20.0f)
#define CAR_RIGHT_SPEED_KI (40.0f)
#define CAR_RIGHT_SPEED_KD (0.0f)
#define CAR_RIGHT_SPEED_I_MAX_DUTY (4500.0f)
#define CAR_RIGHT_SPEED_ACCEL_MM_S2 (1000.0f)

/* Verified static straight-line baseline from the 10 s ground tests. */
#define CAR_STRAIGHT_LEFT_BASE_MM_S  (248.5f)
#define CAR_STRAIGHT_RIGHT_BASE_MM_S (251.5f)

#endif

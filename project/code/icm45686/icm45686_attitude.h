#ifndef __ICM45686_ATTITUDE_H
#define __ICM45686_ATTITUDE_H

#include "zf_common_headfile.h"

typedef struct
{
    float accel_g[3];
    float gyro_dps[3];
    float temperature_c;
} icm45686_data_t;

typedef struct
{
    float yaw;
    float pitch;
    float roll;
    float quaternion[4];
    uint8 calibrated;
    uint8 stationary;
} icm45686_attitude_t;

void icm45686_attitude_init(float sample_hz);
void icm45686_attitude_update(const icm45686_data_t *data, float elapsed_s);
void icm45686_attitude_get(icm45686_attitude_t *attitude);
void icm45686_attitude_reset_yaw(void);
void icm45686_attitude_restart_calibration(void);

#endif

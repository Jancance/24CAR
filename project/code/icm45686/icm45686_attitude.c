#include "icm45686_attitude.h"

#include <math.h>
#include <stddef.h>

#define ICM45686_PI                    (3.14159265358979323846f)
#define ICM45686_DEG_TO_RAD            (ICM45686_PI / 180.0f)
#define ICM45686_RAD_TO_DEG            (180.0f / ICM45686_PI)
#define ICM45686_CALIBRATION_SAMPLES   (200U)
#define ICM45686_STILL_GYRO_DPS        (5.0f)
#define ICM45686_MAHONY_KP             (1.5f)
#define ICM45686_MAHONY_KI             (0.02f)
#define ICM45686_STILL_ACCEL_TOL_G      (0.02f)
#define ICM45686_STILL_RATE_DPS         (0.50f)
#define ICM45686_STILL_CONFIRM_S        (0.50f)
#define ICM45686_BIAS_TRACK_TIME_S      (20.0f)

static float q0;
static float q1;
static float q2;
static float q3;
static float integral_x;
static float integral_y;
static float integral_z;
static float sample_period_s;
static float gyro_bias_dps[3];
static float gyro_sum_dps[3];
static uint16 calibration_count;
static uint8 calibration_complete;
static float yaw_zero_deg;
static float yaw_absolute_deg;
static float yaw_rate_dps;
static float stationary_time_s;

static float icm45686_wrap_angle(float angle)
{
    while (angle > 180.0f)
    {
        angle -= 360.0f;
    }
    while (angle < -180.0f)
    {
        angle += 360.0f;
    }
    return angle;
}

static void icm45686_update_calibration(const icm45686_data_t *data)
{
    float accel_norm;

    if (calibration_complete)
    {
        return;
    }

    accel_norm = sqrtf(data->accel_g[0] * data->accel_g[0] +
                       data->accel_g[1] * data->accel_g[1] +
                       data->accel_g[2] * data->accel_g[2]);

    if ((accel_norm < 0.85f) || (accel_norm > 1.15f) ||
        (fabsf(data->gyro_dps[0]) > ICM45686_STILL_GYRO_DPS) ||
        (fabsf(data->gyro_dps[1]) > ICM45686_STILL_GYRO_DPS) ||
        (fabsf(data->gyro_dps[2]) > ICM45686_STILL_GYRO_DPS))
    {
        calibration_count = 0U;
        gyro_sum_dps[0] = 0.0f;
        gyro_sum_dps[1] = 0.0f;
        gyro_sum_dps[2] = 0.0f;
        return;
    }

    gyro_sum_dps[0] += data->gyro_dps[0];
    gyro_sum_dps[1] += data->gyro_dps[1];
    gyro_sum_dps[2] += data->gyro_dps[2];
    calibration_count++;

    if (calibration_count >= ICM45686_CALIBRATION_SAMPLES)
    {
        gyro_bias_dps[0] = gyro_sum_dps[0] / (float)calibration_count;
        gyro_bias_dps[1] = gyro_sum_dps[1] / (float)calibration_count;
        gyro_bias_dps[2] = gyro_sum_dps[2] / (float)calibration_count;
        calibration_complete = 1U;
        integral_x = 0.0f;
        integral_y = 0.0f;
        integral_z = 0.0f;
    }
}

void icm45686_attitude_init(float sample_hz)
{
    q0 = 1.0f;
    q1 = 0.0f;
    q2 = 0.0f;
    q3 = 0.0f;
    integral_x = 0.0f;
    integral_y = 0.0f;
    integral_z = 0.0f;
    sample_period_s = (sample_hz > 0.0f) ? (1.0f / sample_hz) : 0.005f;
    yaw_zero_deg = 0.0f;
    yaw_absolute_deg = 0.0f;
    yaw_rate_dps = 0.0f;
    stationary_time_s = 0.0f;
    icm45686_attitude_restart_calibration();
}

void icm45686_attitude_restart_calibration(void)
{
    gyro_bias_dps[0] = 0.0f;
    gyro_bias_dps[1] = 0.0f;
    gyro_bias_dps[2] = 0.0f;
    gyro_sum_dps[0] = 0.0f;
    gyro_sum_dps[1] = 0.0f;
    gyro_sum_dps[2] = 0.0f;
    calibration_count = 0U;
    calibration_complete = 0U;
    stationary_time_s = 0.0f;
}

void icm45686_attitude_update(const icm45686_data_t *data, float elapsed_s)
{
    float ax;
    float ay;
    float az;
    float gx;
    float gy;
    float gz;
    float gx_dps;
    float gy_dps;
    float gz_dps;
    float bias_track_alpha;
    float norm;
    float vx;
    float vy;
    float vz;
    float ex;
    float ey;
    float ez;
    float half_dt;
    float nq0;
    float nq1;
    float nq2;
    float nq3;
    float previous_yaw_absolute_deg;
    uint8 calibration_was_complete;

    if (data == NULL)
    {
        return;
    }

    // OLED/串口刷新会阻塞主循环，不能假定每次调用都严格相隔 5 ms。
    // 使用实测间隔可避免偏航角只累计实际转角的一部分。
    if ((elapsed_s > 0.0005f) && (elapsed_s < 0.2000f))
    {
        sample_period_s = elapsed_s;
    }

    previous_yaw_absolute_deg = yaw_absolute_deg;
    calibration_was_complete = calibration_complete;
    icm45686_update_calibration(data);

    ax = data->accel_g[0];
    ay = data->accel_g[1];
    az = data->accel_g[2];
    gx_dps = data->gyro_dps[0] - gyro_bias_dps[0];
    gy_dps = data->gyro_dps[1] - gyro_bias_dps[1];
    gz_dps = data->gyro_dps[2] - gyro_bias_dps[2];
    yaw_rate_dps = calibration_complete ? gz_dps : 0.0f;

    norm = sqrtf(ax * ax + ay * ay + az * az);
    if (calibration_complete &&
        (fabsf(norm - 1.0f) < ICM45686_STILL_ACCEL_TOL_G) &&
        (fabsf(gx_dps) < ICM45686_STILL_RATE_DPS) &&
        (fabsf(gy_dps) < ICM45686_STILL_RATE_DPS) &&
        (fabsf(gz_dps) < ICM45686_STILL_RATE_DPS))
    {
        stationary_time_s += sample_period_s;
        if (stationary_time_s >= ICM45686_STILL_CONFIRM_S)
        {
            // 静止时缓慢跟踪温漂零偏，并禁止微小残余 GZ 累计到 Yaw。
            bias_track_alpha = sample_period_s /
                               (ICM45686_BIAS_TRACK_TIME_S + sample_period_s);
            gyro_bias_dps[0] += bias_track_alpha * gx_dps;
            gyro_bias_dps[1] += bias_track_alpha * gy_dps;
            gyro_bias_dps[2] += bias_track_alpha * gz_dps;
            gx_dps = data->gyro_dps[0] - gyro_bias_dps[0];
            gy_dps = data->gyro_dps[1] - gyro_bias_dps[1];
            gz_dps = 0.0f;
            yaw_rate_dps = 0.0f;
        }
    }
    else
    {
        stationary_time_s = 0.0f;
    }

    gx = gx_dps * ICM45686_DEG_TO_RAD;
    gy = gy_dps * ICM45686_DEG_TO_RAD;
    gz = gz_dps * ICM45686_DEG_TO_RAD;

    if (norm > 0.001f)
    {
        ax /= norm;
        ay /= norm;
        az /= norm;

        vx = 2.0f * (q1 * q3 - q0 * q2);
        vy = 2.0f * (q0 * q1 + q2 * q3);
        vz = q0 * q0 - q1 * q1 - q2 * q2 + q3 * q3;

        ex = ay * vz - az * vy;
        ey = az * vx - ax * vz;
        ez = ax * vy - ay * vx;

        if (calibration_complete)
        {
            integral_x += ICM45686_MAHONY_KI * ex * sample_period_s;
            integral_y += ICM45686_MAHONY_KI * ey * sample_period_s;
            integral_z += ICM45686_MAHONY_KI * ez * sample_period_s;
        }

        gx += ICM45686_MAHONY_KP * ex + integral_x;
        gy += ICM45686_MAHONY_KP * ey + integral_y;
        gz += ICM45686_MAHONY_KP * ez + integral_z;
    }

    half_dt = 0.5f * sample_period_s;
    nq0 = q0 + (-q1 * gx - q2 * gy - q3 * gz) * half_dt;
    nq1 = q1 + ( q0 * gx + q2 * gz - q3 * gy) * half_dt;
    nq2 = q2 + ( q0 * gy - q1 * gz + q3 * gx) * half_dt;
    nq3 = q3 + ( q0 * gz + q1 * gy - q2 * gx) * half_dt;

    norm = sqrtf(nq0 * nq0 + nq1 * nq1 + nq2 * nq2 + nq3 * nq3);
    if (norm > 0.001f)
    {
        q0 = nq0 / norm;
        q1 = nq1 / norm;
        q2 = nq2 / norm;
        q3 = nq3 / norm;
    }

    yaw_absolute_deg = atan2f(2.0f * (q0 * q3 + q1 * q2),
                              1.0f - 2.0f * (q2 * q2 + q3 * q3)) *
                       ICM45686_RAD_TO_DEG;

    // 启动校准阶段尚未得到陀螺零偏，Yaw 会累计一小段启动偏移。
    // 校准完成的瞬间自动以当前方向为零，避免复位后停在约 1 度。
    if ((!calibration_was_complete) && calibration_complete)
    {
        yaw_zero_deg = yaw_absolute_deg;
    }
    else if (calibration_complete &&
             (stationary_time_s >= ICM45686_STILL_CONFIRM_S))
    {
        // 重力校正 Pitch/Roll 时会通过四元数产生极小的 Euler Yaw 耦合。
        // 静止期间同步移动零点来抵消该增量，但保留停车前的航向角。
        yaw_zero_deg = icm45686_wrap_angle(
            yaw_zero_deg +
            icm45686_wrap_angle(yaw_absolute_deg - previous_yaw_absolute_deg));
    }
}

void icm45686_attitude_get(icm45686_attitude_t *attitude)
{
    float pitch_sin;

    if (attitude == NULL)
    {
        return;
    }

    pitch_sin = 2.0f * (q0 * q2 - q3 * q1);
    if (pitch_sin > 1.0f)
    {
        pitch_sin = 1.0f;
    }
    else if (pitch_sin < -1.0f)
    {
        pitch_sin = -1.0f;
    }

    attitude->yaw = icm45686_wrap_angle(yaw_absolute_deg - yaw_zero_deg);
    attitude->yaw_rate_dps = yaw_rate_dps;
    attitude->pitch = asinf(pitch_sin) * ICM45686_RAD_TO_DEG;
    attitude->roll = atan2f(2.0f * (q0 * q1 + q2 * q3),
                            1.0f - 2.0f * (q1 * q1 + q2 * q2)) *
                     ICM45686_RAD_TO_DEG;
    attitude->quaternion[0] = q0;
    attitude->quaternion[1] = q1;
    attitude->quaternion[2] = q2;
    attitude->quaternion[3] = q3;
    attitude->calibrated = calibration_complete;
    attitude->stationary = (uint8)(calibration_complete &&
                                    (stationary_time_s >= ICM45686_STILL_CONFIRM_S));
}

void icm45686_attitude_reset_yaw(void)
{
    yaw_zero_deg = yaw_absolute_deg;
}

#include "position_control.h"
#include "board_pins.h"

#include <stddef.h>

#define POSITION_CONTROL_PERIOD_MS          (20U)
#define POSITION_CONTROL_DISPLAY_MS         (100U)
#define POSITION_CONTROL_MAX_TRIM_MM_S      (60.0f)
#define POSITION_CONTROL_KP_MIN             (0.0f)
#define POSITION_CONTROL_KP_MAX             (20.0f)
#define POSITION_CONTROL_KD_MIN             (0.0f)
#define POSITION_CONTROL_KD_MAX             (20.0f)
#define POSITION_CONTROL_SPEED_MIN_MM_S     (50.0f)
#define POSITION_CONTROL_SPEED_MAX_MM_S     (400.0f)
#define POSITION_CONTROL_KEY_DOUBLE_MS      (350U)

typedef enum
{
    POSITION_CONTROL_EDIT_KP = 0,
    POSITION_CONTROL_EDIT_KD,
    POSITION_CONTROL_EDIT_SPEED
} position_control_edit_t;

static position_control_state_t control_state;
static position_control_edit_t edit_parameter;
static float tune_base_speed_mm_s;
static uint32 control_ms;
static uint32 display_ms;
static uint8 pending_adjust_key;
static uint32 pending_adjust_ms;

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

static void position_control_show_signed(uint8 x, uint8 y, float value)
{
    uint32 magnitude;

    if (value < 0.0f)
    {
        OLED_ShowChar(x, y, '-', 8U);
        value = -value;
    }
    else
    {
        OLED_ShowChar(x, y, '+', 8U);
    }
    magnitude = (uint32)(value + 0.5f);
    OLED_ShowNum((uint8)(x + 6U), y,
                 (magnitude > 999U) ? 999U : magnitude, 3U, 8U);
}

static void position_control_update_oled(void)
{
    OLED_ShowString(0U, 0U, (uint8 *)"YAW OUTER LOOP", 8U);
    OLED_ShowString(0U, 1U, (uint8 *)"CAL:", 8U);
    OLED_ShowString(30U, 1U,
                    control_state.icm_calibrated ?
                        (uint8 *)"OK  " : (uint8 *)"WAIT", 8U);
    OLED_ShowString(0U, 2U, (uint8 *)"YAW:", 8U);
    position_control_show_signed(30U, 2U, control_state.yaw_deg);
    OLED_ShowString(0U, 3U, (uint8 *)"RATE:", 8U);
    position_control_show_signed(36U, 3U, control_state.yaw_rate_dps);
    OLED_ShowString(0U, 4U, (uint8 *)"TRIM:", 8U);
    position_control_show_signed(36U, 4U, control_state.trim_mm_s);
    OLED_ShowString(0U, 5U, (uint8 *)"P:", 8U);
    OLED_ShowNum(12U, 5U, (uint32)(control_state.kp * 100.0f + 0.5f), 4U, 8U);
    OLED_ShowString(42U, 5U, (uint8 *)"D:", 8U);
    OLED_ShowNum(54U, 5U, (uint32)(control_state.kd * 100.0f + 0.5f), 4U, 8U);
    OLED_ShowString(0U, 6U, (uint8 *)"EDIT:", 8U);
    OLED_ShowChar(30U, 6U,
                  (edit_parameter == POSITION_CONTROL_EDIT_KP) ? 'P' :
                  ((edit_parameter == POSITION_CONTROL_EDIT_KD) ? 'D' : 'S'),
                  8U);
    OLED_ShowString(42U, 6U, (uint8 *)"SPD:", 8U);
    OLED_ShowNum(66U, 6U, (uint32)(tune_base_speed_mm_s + 0.5f), 3U, 8U);
    OLED_ShowString(0U, 7U, (uint8 *)(control_state.active ?
                                     "RUN OK:STOP" : "STOP OK:RUN"), 8U);
}

static void position_control_adjust(float parameter_delta, float speed_delta)
{
    if (edit_parameter == POSITION_CONTROL_EDIT_KP)
    {
        control_state.kp = position_control_clamp(
            control_state.kp + parameter_delta,
            POSITION_CONTROL_KP_MIN, POSITION_CONTROL_KP_MAX);
    }
    else if (edit_parameter == POSITION_CONTROL_EDIT_KD)
    {
        control_state.kd = position_control_clamp(
            control_state.kd + parameter_delta,
            POSITION_CONTROL_KD_MIN, POSITION_CONTROL_KD_MAX);
    }
    else
    {
        tune_base_speed_mm_s = position_control_clamp(
            tune_base_speed_mm_s + speed_delta,
            POSITION_CONTROL_SPEED_MIN_MM_S,
            POSITION_CONTROL_SPEED_MAX_MM_S);
    }
}

static void position_control_select_next(void)
{
    if (edit_parameter == POSITION_CONTROL_EDIT_KP)
    {
        edit_parameter = POSITION_CONTROL_EDIT_KD;
    }
    else if (edit_parameter == POSITION_CONTROL_EDIT_KD)
    {
        edit_parameter = POSITION_CONTROL_EDIT_SPEED;
    }
    else
    {
        edit_parameter = POSITION_CONTROL_EDIT_KP;
    }
}

static void position_control_handle_adjust_key(uint8 key, uint32 now_ms)
{
    if ((pending_adjust_key == key) &&
        ((uint32)(now_ms - pending_adjust_ms) <= POSITION_CONTROL_KEY_DOUBLE_MS))
    {
        position_control_adjust((key == 2U) ? 0.05f : -0.05f,
                                (key == 2U) ? 100.0f : -100.0f);
        pending_adjust_key = 0U;
        return;
    }

    if ((pending_adjust_key != 0U) &&
        ((uint32)(now_ms - pending_adjust_ms) > POSITION_CONTROL_KEY_DOUBLE_MS))
    {
        position_control_adjust((pending_adjust_key == 2U) ? 0.01f : -0.01f,
                                (pending_adjust_key == 2U) ? 20.0f : -20.0f);
    }
    pending_adjust_key = key;
    pending_adjust_ms = now_ms;
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
    tune_base_speed_mm_s = CAR_YAW_POSITION_START_SPEED_MM_S;
    edit_parameter = POSITION_CONTROL_EDIT_KP;
    pending_adjust_key = 0U;
    control_ms = system_get_ms();
    display_ms = control_ms;
    pending_adjust_ms = control_ms;
    OLED_Clear();
    position_control_update_oled();
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
    speed_control_set_target_trim(base_speed_mm_s, 0.0f);
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
    uint8 key;
    float trim;

    (void)icm45686_update();
    icm45686_get_attitude(&attitude);
    control_state.icm_calibrated = attitude.calibrated;
    control_state.yaw_deg = attitude.yaw;
    control_state.yaw_rate_dps = attitude.yaw_rate_dps;
    now_ms = system_get_ms();

    key = Key_GetNum();
    if (!control_state.active)
    {
        if (key == 1U)
        {
            pending_adjust_key = 0U;
            position_control_select_next();
        }
        else if ((key == 2U) || (key == 3U))
        {
            position_control_handle_adjust_key(key, now_ms);
        }
    }
    if (!control_state.active && (pending_adjust_key != 0U) &&
        ((uint32)(now_ms - pending_adjust_ms) > POSITION_CONTROL_KEY_DOUBLE_MS))
    {
        position_control_adjust((pending_adjust_key == 2U) ? 0.01f : -0.01f,
                                (pending_adjust_key == 2U) ? 20.0f : -20.0f);
        pending_adjust_key = 0U;
    }
    if (key == 4U)
    {
        pending_adjust_key = 0U;
        if (control_state.active)
        {
            position_control_stop();
        }
        else
        {
            (void)position_control_start(tune_base_speed_mm_s);
        }
    }

    if ((uint32)(now_ms - display_ms) >= POSITION_CONTROL_DISPLAY_MS)
    {
        display_ms = now_ms;
        position_control_update_oled();
    }
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
    control_state.trim_mm_s = position_control_clamp(
        trim, -POSITION_CONTROL_MAX_TRIM_MM_S, POSITION_CONTROL_MAX_TRIM_MM_S);
    speed_control_set_target_trim(control_state.base_speed_mm_s,
                                  control_state.trim_mm_s);
}

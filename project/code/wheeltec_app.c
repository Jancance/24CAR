#include "wheeltec_app.h"
#include "board_pins.h"

#include <stdlib.h>
#include <string.h>

#define WHEELTEC_FRAME_MAX       (32U)
#define WHEELTEC_STREAM_PERIOD_MS (50U)
#define WHEELTEC_TARGET_MIN_MM_S (100.0f)
#define WHEELTEC_TARGET_MAX_MM_S (400.0f)
#define WHEELTEC_SPEED_STEP_MM_S (50.0f)
#define WHEELTEC_KP_SCALE        (50.0f)
#define WHEELTEC_KI_SCALE        (100.0f)
#define WHEELTEC_KD_SCALE        (50.0f)

static char wheeltec_frame[WHEELTEC_FRAME_MAX];
static uint8 wheeltec_frame_length;
static uint8 wheeltec_frame_active;
static uint8 wheeltec_expect_suffix;
static uint8 wheeltec_stream_enabled;
static uint8 wheeltec_remote_enabled;
static uint32 wheeltec_stream_ms;
static float wheeltec_target_mm_s;

static float wheeltec_clamp(float value, float minimum, float maximum)
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

static int32 wheeltec_round(float value)
{
    return (int32)(value + ((value >= 0.0f) ? 0.5f : -0.5f));
}

static void wheeltec_send_parameters(void)
{
    line_follow_state_t state;

    line_follow_get_state(&state);
    Serial_Printf("{C%ld:%ld:%ld:%ld:0:0:0:0:0}$",
                  (long)wheeltec_round(state.kp * WHEELTEC_KP_SCALE),
                  (long)wheeltec_round(state.ki * WHEELTEC_KI_SCALE),
                  (long)wheeltec_round(state.kd * WHEELTEC_KD_SCALE),
                  (long)wheeltec_round(wheeltec_target_mm_s));
}

static void wheeltec_set_parameter(uint8 slot, float value)
{
    line_follow_state_t state;

    line_follow_get_state(&state);
    switch (slot)
    {
        case 0U:
            line_follow_set_gains(value / WHEELTEC_KP_SCALE,
                                  state.ki, state.kd);
            break;
        case 1U:
            line_follow_set_gains(state.kp,
                                  value / WHEELTEC_KI_SCALE, state.kd);
            break;
        case 2U:
            line_follow_set_gains(state.kp, state.ki,
                                  value / WHEELTEC_KD_SCALE);
            break;
        case 3U:
            wheeltec_target_mm_s = wheeltec_clamp(
                value, 0.0f, WHEELTEC_TARGET_MAX_MM_S);
            break;
        default:
            return;
    }
    wheeltec_send_parameters();
}

static void wheeltec_parse_frame(void)
{
    char *value_text;

    wheeltec_frame[wheeltec_frame_length] = '\0';
    if ((wheeltec_frame[0] == 'C') &&
        (strchr(wheeltec_frame, 'P') != NULL))
    {
        wheeltec_send_parameters();
        return;
    }
    if ((wheeltec_frame_length >= 3U) &&
        (wheeltec_frame[0] >= '0') && (wheeltec_frame[0] <= '8') &&
        (wheeltec_frame[1] == ':'))
    {
        value_text = &wheeltec_frame[2];
        wheeltec_set_parameter((uint8)(wheeltec_frame[0] - '0'),
                               strtof(value_text, NULL));
    }
}

static void wheeltec_process_remote(uint8 data)
{
    if (task_manager_is_running())
    {
        return;
    }

    if ((data == (uint8)'A') || (data == 1U))
    {
        if (!line_follow_is_active())
        {
            line_follow_start(wheeltec_target_mm_s);
        }
    }
    else if (data == (uint8)'X')
    {
        wheeltec_target_mm_s = wheeltec_clamp(
            wheeltec_target_mm_s + WHEELTEC_SPEED_STEP_MM_S,
            WHEELTEC_TARGET_MIN_MM_S, WHEELTEC_TARGET_MAX_MM_S);
        wheeltec_send_parameters();
    }
    else if (data == (uint8)'Y')
    {
        wheeltec_target_mm_s = wheeltec_clamp(
            wheeltec_target_mm_s - WHEELTEC_SPEED_STEP_MM_S,
            WHEELTEC_TARGET_MIN_MM_S, WHEELTEC_TARGET_MAX_MM_S);
        wheeltec_send_parameters();
    }
    else if ((data == 0U) ||
             ((data >= (uint8)'B') && (data <= (uint8)'H')) ||
             ((data >= 2U) && (data <= 8U)))
    {
        line_follow_stop();
    }
}

void wheeltec_app_init(void)
{
    wheeltec_frame_length = 0U;
    wheeltec_frame_active = 0U;
    wheeltec_expect_suffix = 0U;
    wheeltec_stream_enabled = 0U;
    wheeltec_remote_enabled = 0U;
    wheeltec_stream_ms = system_get_ms();
    wheeltec_target_mm_s = 200.0f;
}

uint8 wheeltec_app_process_byte(uint8 data)
{
    if (data == (uint8)'{')
    {
        wheeltec_frame_length = 0U;
        wheeltec_frame_active = 1U;
        wheeltec_expect_suffix = 0U;
        wheeltec_stream_enabled = 1U;
        return 1U;
    }
    if (wheeltec_frame_active)
    {
        if (data == (uint8)'}')
        {
            wheeltec_frame_active = 0U;
            wheeltec_expect_suffix = 1U;
            wheeltec_parse_frame();
            return 1U;
        }
        if (wheeltec_frame_length < (WHEELTEC_FRAME_MAX - 1U))
        {
            wheeltec_frame[wheeltec_frame_length++] = (char)data;
        }
        else
        {
            wheeltec_frame_length = 0U;
            wheeltec_frame_active = 0U;
        }
        return 1U;
    }
    if (wheeltec_expect_suffix && (data == (uint8)'$'))
    {
        wheeltec_expect_suffix = 0U;
        return 1U;
    }
    wheeltec_expect_suffix = 0U;

    if ((data == (uint8)'I') || (data == (uint8)'J') ||
        (data == (uint8)'K'))
    {
        wheeltec_remote_enabled = 1U;
        wheeltec_stream_enabled = 1U;
        return 1U;
    }
    if (wheeltec_remote_enabled &&
        ((data <= 8U) ||
         ((data >= (uint8)'A') && (data <= (uint8)'H')) ||
         (data == (uint8)'X') || (data == (uint8)'Y')))
    {
        wheeltec_process_remote(data);
        return 1U;
    }
    return 0U;
}

uint8 wheeltec_app_is_stream_enabled(void)
{
    return wheeltec_stream_enabled;
}

void wheeltec_app_toggle_line_follow(void)
{
    if (task_manager_is_running())
    {
        return;
    }
    if (line_follow_is_active())
    {
        line_follow_stop();
    }
    else
    {
        line_follow_start(wheeltec_target_mm_s);
    }
}

void wheeltec_app_loop(void)
{
    speed_control_state_t state;
    line_follow_state_t line_state;
    uint32 now_ms;
    int32 left_show;
    int32 right_show;
    int32 line_actual;
    int32 line_output;

    if (!wheeltec_stream_enabled)
    {
        return;
    }
    now_ms = system_get_ms();
    if ((uint32)(now_ms - wheeltec_stream_ms) < WHEELTEC_STREAM_PERIOD_MS)
    {
        return;
    }
    wheeltec_stream_ms = now_ms;
    speed_control_get_state(&state);
    line_follow_get_state(&line_state);
    left_show = wheeltec_round(state.left_speed_mm_s);
    right_show = wheeltec_round(state.right_speed_mm_s);
    line_actual = wheeltec_round(line_state.error * 100.0f);
    line_output = wheeltec_round(line_state.trim_mm_s);

    Serial_Printf("{A%ld:%ld:%ld:%ld}$",
                  (long)left_show, (long)right_show, 0L, 0L);
    Serial_Printf("{B0:%ld:%ld}$",
                  (long)line_actual, (long)line_output);
}

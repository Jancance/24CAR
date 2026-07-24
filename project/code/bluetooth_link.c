#include "bluetooth_link.h"
#include "board_pins.h"
#include "wheeltec_app.h"

#include <stdlib.h>
#include <string.h>

#define BLUETOOTH_LINK_COMMAND_MAX (24U)
#define BLUETOOTH_TELEMETRY_MS     (50U)
#define LINE_TEST_MIN_DURATION_MS  (200U)
#define LINE_TEST_MAX_DURATION_MS  (10000U)
#define LINE_TEST_MAX_START_ERROR  (0.5f)

typedef struct
{
    uint32 duration_ms;
    uint32 sample_count;
    uint32 found_count;
    uint32 lost_count;
    uint32 crossing_count;
    float absolute_error_sum;
    float minimum_error;
    float maximum_error;
    float previous_error;
    uint8 has_previous_error;
    uint8 completed;
} line_test_result_t;

static char command[BLUETOOTH_LINK_COMMAND_MAX];
static uint8 command_length;
static uint32 yaw_stream_ms;
static uint32 line_stream_ms;
static uint8 line_test_active;
static uint32 line_test_start_ms;
static line_test_result_t line_test_result;

static uint8 bluetooth_link_yaw_is_active(void)
{
    position_control_state_t state;

    position_control_get_state(&state);
    return state.active;
}

static void bluetooth_link_send_yaw(void)
{
    position_control_state_t state;
    speed_control_state_t speed_state;

    position_control_get_state(&state);
    speed_control_get_state(&speed_state);
    Serial_Printf("YAW,CAL=%u,ACT=%u,TGT=%.2f,HDG=%.2f,ERR=%.2f,RATE=%.2f,OUT=%.2f,P=%.2f,D=%.2f,LT=%.1f,RT=%.1f,LS=%.1f,RS=%.1f\r\n",
                  state.icm_calibrated, state.active,
                  state.target_yaw_deg, state.yaw_deg,
                  state.yaw_error_deg, state.yaw_rate_dps,
                  state.trim_mm_s, state.kp, state.kd,
                  speed_state.left_setpoint_mm_s,
                  speed_state.right_setpoint_mm_s,
                  speed_state.left_speed_mm_s,
                  speed_state.right_speed_mm_s);
}

static void bluetooth_link_send_speed(void)
{
    speed_control_state_t state;

    speed_control_get_state(&state);
    Serial_Printf("SPEED,RUN=%u,RAMP=%u,DT=%lu,LT=%.1f,RT=%.1f,LS=%.1f,RS=%.1f,LR=%.1f,RR=%.1f,LRAW=%.1f,RRAW=%.1f\r\n",
                  state.running, state.ramp_enabled,
                  (unsigned long)state.loop_dt_ms, state.left_target_mm_s,
                  state.right_target_mm_s, state.left_setpoint_mm_s,
                  state.right_setpoint_mm_s, state.left_speed_mm_s,
                  state.right_speed_mm_s, state.left_raw_mm_s,
                  state.right_raw_mm_s);
    Serial_Printf("PWM,LD=%ld,RD=%ld\r\n",
                  (int32)state.left_duty, (int32)state.right_duty);
}

static void bluetooth_link_send_line(void)
{
    line_follow_state_t state;

    line_follow_get_state(&state);
    Serial_Printf("LINE,ACT=%u,FOUND=%u,MASK=%u,ERR=%.2f,IERR=%.2f,DERR=%.2f,SPD=%.1f,TRIM=%.1f,P=%.2f,I=%.2f,D=%.2f\r\n",
                  state.active, state.line_detected, state.active_mask,
                  state.error, state.integral, state.derivative,
                  state.commanded_speed_mm_s, state.trim_mm_s,
                  state.kp, state.ki, state.kd);
}

static void bluetooth_link_reset_line_test(uint32 duration_ms)
{
    memset(&line_test_result, 0, sizeof(line_test_result));
    line_test_result.duration_ms = duration_ms;
}

static void bluetooth_link_record_line_test(void)
{
    line_follow_state_t state;
    float absolute_error;

    line_follow_get_state(&state);
    line_test_result.sample_count++;
    if (!state.line_detected)
    {
        line_test_result.lost_count++;
        return;
    }

    absolute_error = (state.error < 0.0f) ? -state.error : state.error;
    line_test_result.absolute_error_sum += absolute_error;
    if (line_test_result.found_count == 0U)
    {
        line_test_result.minimum_error = state.error;
        line_test_result.maximum_error = state.error;
    }
    else
    {
        if (state.error < line_test_result.minimum_error)
        {
            line_test_result.minimum_error = state.error;
        }
        if (state.error > line_test_result.maximum_error)
        {
            line_test_result.maximum_error = state.error;
        }
        if (line_test_result.has_previous_error &&
            (state.error * line_test_result.previous_error < 0.0f))
        {
            line_test_result.crossing_count++;
        }
    }
    line_test_result.previous_error = state.error;
    line_test_result.has_previous_error = 1U;
    line_test_result.found_count++;
}

static void bluetooth_link_send_line_result(void)
{
    float average_absolute_error = 0.0f;

    if (line_test_result.found_count > 0U)
    {
        average_absolute_error = line_test_result.absolute_error_sum /
                                 (float)line_test_result.found_count;
    }
    Serial_Printf("LINE_RESULT,DONE=%u,DUR=%lu,N=%lu,FOUND=%lu,LOST=%lu,ABS=%.2f,MIN=%.2f,MAX=%.2f,CROSS=%lu\r\n",
                  line_test_result.completed,
                  (unsigned long)line_test_result.duration_ms,
                  (unsigned long)line_test_result.sample_count,
                  (unsigned long)line_test_result.found_count,
                  (unsigned long)line_test_result.lost_count,
                  average_absolute_error,
                  line_test_result.minimum_error,
                  line_test_result.maximum_error,
                  (unsigned long)line_test_result.crossing_count);
}

static void bluetooth_link_process_command(void)
{
    command[command_length] = '\0';

    if (strcmp(command, "PING") == 0)
    {
        Serial_SendString("PONG\r\n");
    }
    else if (strcmp(command, "YAW GET") == 0)
    {
        bluetooth_link_send_yaw();
    }
    else if (strcmp(command, "INFO") == 0)
    {
        Serial_Printf("BT,UART3,%lu,8N1\r\n",
                      (unsigned long)CAR_DEBUG_UART_BAUD);
    }
    else if (strcmp(command, "SPEED GET") == 0)
    {
        bluetooth_link_send_speed();
    }
    else if (strcmp(command, "LINE GET") == 0)
    {
        bluetooth_link_send_line();
    }
    else if (strcmp(command, "LINE CHECK") == 0)
    {
        float error = 0.0f;
        uint8 mask = 0U;
        uint8 found = line_follow_sample_now(&error, &mask);

        Serial_Printf("LINE_CHECK,FOUND=%u,MASK=%u,ERR=%.2f\r\n",
                      found, mask, error);
    }
    else if (strcmp(command, "LINE RESULT") == 0)
    {
        bluetooth_link_send_line_result();
    }
    else if (strcmp(command, "LINE STOP") == 0)
    {
        line_test_active = 0U;
        line_follow_stop();
        Serial_SendString("OK,LINE_STOP\r\n");
    }
    else if (strncmp(command, "LINE P ", 7U) == 0)
    {
        line_follow_state_t state;

        line_follow_get_state(&state);
        if (state.active)
        {
            Serial_SendString("ERR,LINE_STOP_FIRST\r\n");
        }
        else
        {
            line_follow_set_gains(strtof(command + 7, NULL),
                                  state.ki, state.kd);
            Serial_SendString("OK,LINE_P\r\n");
        }
    }
    else if (strncmp(command, "LINE I ", 7U) == 0)
    {
        line_follow_state_t state;

        line_follow_get_state(&state);
        if (state.active)
        {
            Serial_SendString("ERR,LINE_STOP_FIRST\r\n");
        }
        else
        {
            line_follow_set_gains(state.kp,
                                  strtof(command + 7, NULL), state.kd);
            Serial_SendString("OK,LINE_I\r\n");
        }
    }
    else if (strncmp(command, "LINE D ", 7U) == 0)
    {
        line_follow_state_t state;

        line_follow_get_state(&state);
        if (state.active)
        {
            Serial_SendString("ERR,LINE_STOP_FIRST\r\n");
        }
        else
        {
            line_follow_set_gains(state.kp, state.ki,
                                  strtof(command + 7, NULL));
            Serial_SendString("OK,LINE_D\r\n");
        }
    }
    else if (strncmp(command, "LINE START ", 11U) == 0)
    {
        float speed = strtof(command + 11, NULL);
        line_test_active = 0U;
        line_stream_ms = system_get_ms();
        Serial_SendString(line_follow_start(speed) ?
                          "OK,LINE_START\r\n" : "ERR,LINE_START\r\n");
    }
    else if (strncmp(command, "LINE TEST ", 10U) == 0)
    {
        char *duration_value;
        float speed;
        float start_error = 0.0f;
        uint32 duration_ms;
        uint8 start_mask = 0U;
        uint8 start_found;

        speed = strtof(command + 10, &duration_value);
        duration_ms = (uint32)strtoul(duration_value, NULL, 10);
        start_found = line_follow_sample_now(&start_error, &start_mask);
        if ((duration_ms < LINE_TEST_MIN_DURATION_MS) ||
            (duration_ms > LINE_TEST_MAX_DURATION_MS))
        {
            Serial_SendString("ERR,LINE_TEST_DURATION\r\n");
        }
        else if (task_manager_is_running())
        {
            Serial_SendString("ERR,TASK_RUNNING\r\n");
        }
        else if (!start_found ||
                 (start_error < -LINE_TEST_MAX_START_ERROR) ||
                 (start_error > LINE_TEST_MAX_START_ERROR))
        {
            Serial_Printf("ERR,LINE_TEST_ALIGN,MASK=%u,ERR=%.2f\r\n",
                          start_mask, start_error);
        }
        else
        {
            bluetooth_link_reset_line_test(duration_ms);
            line_test_start_ms = system_get_ms();
            line_stream_ms = line_test_start_ms;
            line_test_active = line_follow_start(speed);
            Serial_SendString(line_test_active ?
                              "OK,LINE_TEST_START\r\n" :
                              "ERR,LINE_TEST_START\r\n");
        }
    }
    else if (strcmp(command, "DIST") == 0)
    {
        float left_mm;
        float right_mm;

        car_encoder_get_distance_mm(&left_mm, &right_mm);
        Serial_Printf("DIST,L=%.1f,R=%.1f,DIFF=%.1f\r\n",
                      left_mm, right_mm, right_mm - left_mm);
    }
    else if (strcmp(command, "STOP") == 0)
    {
        line_test_active = 0U;
        task_manager_abort();
        Serial_SendString("OK,STOP\r\n");
    }
    else if (command_length > 0U)
    {
        Serial_SendString("ERR,COMMAND\r\n");
    }
    command_length = 0U;
}

void bluetooth_link_init(void)
{
    command_length = 0U;
    line_test_active = 0U;
    line_test_start_ms = system_get_ms();
    bluetooth_link_reset_line_test(0U);
    yaw_stream_ms = system_get_ms();
    line_stream_ms = yaw_stream_ms;
    wheeltec_app_init();
    Serial_Printf("BT READY,UART3,%lu,8N1\r\n",
                  (unsigned long)CAR_DEBUG_UART_BAUD);
}

void bluetooth_link_loop(void)
{
    uint8 data;
    uint32 now_ms;

    if (Serial_GetRxOverflow())
    {
        command_length = 0U;
        Serial_SendString("ERR,RX_OVERFLOW\r\n");
    }

    while (Serial_GetRxFlag())
    {
        data = Serial_GetRxData();
        /* Once an ASCII command has started, keep its remaining bytes away
         * from Wheeltec's single-byte I/J/K and A-H remote commands. */
        if ((command_length == 0U) && wheeltec_app_process_byte(data))
        {
            continue;
        }
        if ((data == '\r') || (data == '\n'))
        {
            bluetooth_link_process_command();
        }
        else if ((data >= 0x20U) && (data <= 0x7EU))
        {
            if (command_length < (BLUETOOTH_LINK_COMMAND_MAX - 1U))
            {
                command[command_length++] = (char)data;
            }
            else
            {
                command_length = 0U;
                Serial_SendString("ERR,TOO_LONG\r\n");
            }
        }
    }

    now_ms = system_get_ms();
    if (line_test_active &&
        (uint32)(now_ms - line_test_start_ms) >= line_test_result.duration_ms)
    {
        line_follow_stop();
        line_test_active = 0U;
        line_test_result.completed = 1U;
        Serial_SendString("OK,LINE_TEST_DONE\r\n");
        bluetooth_link_send_line_result();
    }
    if (bluetooth_link_yaw_is_active() &&
        (uint32)(now_ms - yaw_stream_ms) >= BLUETOOTH_TELEMETRY_MS)
    {
        yaw_stream_ms = now_ms;
        bluetooth_link_send_yaw();
    }
    if (line_follow_is_active() &&
        (uint32)(now_ms - line_stream_ms) >= BLUETOOTH_TELEMETRY_MS)
    {
        line_stream_ms = now_ms;
        if (line_test_active)
        {
            bluetooth_link_record_line_test();
        }
        if (!wheeltec_app_is_stream_enabled())
        {
            bluetooth_link_send_line();
        }
    }
    wheeltec_app_loop();
}

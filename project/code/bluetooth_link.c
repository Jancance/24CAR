#include "bluetooth_link.h"
#include "board_pins.h"

#include <stdlib.h>
#include <string.h>

#define BLUETOOTH_LINK_COMMAND_MAX (24U)

static char command[BLUETOOTH_LINK_COMMAND_MAX];
static uint8 command_length;

static uint8 bluetooth_link_is_running(void)
{
    speed_control_state_t state;

    speed_control_get_state(&state);
    return state.running;
}

static uint8 bluetooth_link_yaw_is_active(void)
{
    position_control_state_t state;

    position_control_get_state(&state);
    return state.active;
}

static void bluetooth_link_send_yaw(void)
{
    position_control_state_t state;

    position_control_get_state(&state);
    Serial_Printf("YAW,CAL=%u,ACT=%u,HDG=%.2f,RATE=%.2f,TRIM=%.2f,P=%.2f,D=%.2f\r\n",
                  state.icm_calibrated, state.active, state.yaw_deg,
                  state.yaw_rate_dps, state.trim_mm_s,
                  state.kp, state.kd);
}

static void bluetooth_link_process_yaw_command(void)
{
    position_control_state_t state;

    position_control_get_state(&state);
    if (strcmp(command, "YAW GET") == 0)
    {
        bluetooth_link_send_yaw();
    }
    else if (strcmp(command, "YAW STOP") == 0)
    {
        position_control_stop();
        Serial_SendString("OK,YAW_STOP\r\n");
    }
    else if (strncmp(command, "YAW START ", 10U) == 0)
    {
        float speed = strtof(command + 10, NULL);
        Serial_SendString(position_control_start(speed) ?
                          "OK,YAW_START\r\n" : "ERR,ICM_NOT_READY\r\n");
    }
    else if (strncmp(command, "YAW P ", 6U) == 0)
    {
        if (state.active)
        {
            Serial_SendString("ERR,YAW_STOP_FIRST\r\n");
        }
        else
        {
            position_control_set_gains(strtof(command + 6, NULL), state.kd);
            Serial_SendString("OK,YAW_P\r\n");
        }
    }
    else if (strncmp(command, "YAW D ", 6U) == 0)
    {
        if (state.active)
        {
            Serial_SendString("ERR,YAW_STOP_FIRST\r\n");
        }
        else
        {
            position_control_set_gains(state.kp, strtof(command + 6, NULL));
            Serial_SendString("OK,YAW_D\r\n");
        }
    }
    else
    {
        Serial_SendString("ERR,YAW_FORMAT\r\n");
    }
}

static void bluetooth_link_send_pid(void)
{
    float lkp;
    float lki;
    float lkd;
    float rkp;
    float rki;
    float rkd;

    speed_control_get_gains(&lkp, &lki, &lkd, &rkp, &rki, &rkd);
    Serial_Printf("PID,LKP=%.3f,LKI=%.3f,LKD=%.3f,RKP=%.3f,RKI=%.3f,RKD=%.3f\r\n",
                  lkp, lki, lkd, rkp, rki, rkd);
}

static void bluetooth_link_send_speed(void)
{
    speed_control_state_t state;

    speed_control_get_state(&state);
    Serial_Printf("SPEED,RUN=%u,LT=%.1f,RT=%.1f,LS=%.1f,RS=%.1f,LR=%.1f,RR=%.1f\r\n",
                  state.running, state.left_target_mm_s,
                  state.right_target_mm_s, state.left_setpoint_mm_s,
                  state.right_setpoint_mm_s, state.left_speed_mm_s,
                  state.right_speed_mm_s, state.left_raw_mm_s,
                  state.right_raw_mm_s);
    Serial_Printf("PWM,LD=%ld,RD=%ld\r\n",
                  (int32)state.left_duty, (int32)state.right_duty);
}

static uint8 bluetooth_link_set_gain(const char *name, float value)
{
    float lkp;
    float lki;
    float lkd;
    float rkp;
    float rki;
    float rkd;

    if (bluetooth_link_is_running())
    {
        Serial_SendString("ERR,STOP_FIRST\r\n");
        return 1U;
    }
    speed_control_get_gains(&lkp, &lki, &lkd, &rkp, &rki, &rkd);
    if (strcmp(name, "LKP") == 0) {
        lkp = value;
    } else if (strcmp(name, "LKI") == 0) {
        lki = value;
    } else if (strcmp(name, "LKD") == 0) {
        lkd = value;
    } else if (strcmp(name, "RKP") == 0) {
        rkp = value;
    } else if (strcmp(name, "RKI") == 0) {
        rki = value;
    } else if (strcmp(name, "RKD") == 0) {
        rkd = value;
    } else {
        return 0U;
    }
    speed_control_set_left_gains(lkp, lki, lkd);
    speed_control_set_right_gains(rkp, rki, rkd);
    Serial_SendString("OK,PID\r\n");
    return 1U;
}

static void bluetooth_link_process_command(void)
{
    command[command_length] = '\0';

    if (strcmp(command, "PING") == 0)
    {
        Serial_SendString("PONG\r\n");
    }
    else if (strncmp(command, "YAW ", 4U) == 0)
    {
        bluetooth_link_process_yaw_command();
    }
    else if (strcmp(command, "INFO") == 0)
    {
        Serial_Printf("BT,UART3,%lu,8N1\r\n",
                      (unsigned long)CAR_DEBUG_UART_BAUD);
    }
    else if (strcmp(command, "GET PID") == 0)
    {
        bluetooth_link_send_pid();
    }
    else if (strcmp(command, "SPEED GET") == 0)
    {
        bluetooth_link_send_speed();
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
        position_control_stop();
        Serial_SendString("OK,STOP\r\n");
    }
    else if (strncmp(command, "START ", 6U) == 0)
    {
        Serial_SendString("ERR,YAW_MODE\r\n");
    }
    else if (strncmp(command, "SET ", 4U) == 0)
    {
        char name[4];
        float value;
        if ((command_length >= 9U) && (command[7] == ' '))
        {
            name[0] = command[4]; name[1] = command[5];
            name[2] = command[6]; name[3] = '\0';
            value = strtof(command + 8, NULL);
            if (!bluetooth_link_set_gain(name, value))
            {
                Serial_SendString("ERR,PARAM\r\n");
            }
        }
        else
        {
            Serial_SendString("ERR,FORMAT\r\n");
        }
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
    Serial_Printf("BT READY,UART3,%lu,8N1\r\n",
                  (unsigned long)CAR_DEBUG_UART_BAUD);
}

void bluetooth_link_loop(void)
{
    uint8 data;

    if (Serial_GetRxOverflow())
    {
        command_length = 0U;
        Serial_SendString("ERR,RX_OVERFLOW\r\n");
    }

    while (Serial_GetRxFlag())
    {
        data = Serial_GetRxData();
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
}

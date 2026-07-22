#include "speed_tune.h"
#include "board_pins.h"

#define SPEED_TUNE_TARGET_COUNT   (4U)
#define SPEED_TUNE_TEST_MS        (5000U)
#define SPEED_TUNE_STABLE_MS      (2000U)
#define SPEED_TUNE_TELEMETRY_MS   (20U)
#define SPEED_TUNE_DISPLAY_MS     (100U)

static const uint16 speed_tune_targets[SPEED_TUNE_TARGET_COUNT] = {
    100U, 200U, 300U, 400U
};

static uint8 speed_tune_target_index;
static uint8 speed_tune_running;
static uint32 speed_tune_start_ms;
static uint32 speed_tune_telemetry_ms;
static uint32 speed_tune_display_ms;
static uint32 speed_tune_left_sum;
static uint32 speed_tune_right_sum;
static uint16 speed_tune_sample_count;
static uint16 speed_tune_left_peak_duty;
static uint16 speed_tune_right_peak_duty;
static uint16 speed_tune_left_average;
static uint16 speed_tune_right_average;

static int32 speed_tune_round(float value)
{
    return (int32)(value + ((value >= 0.0f) ? 0.5f : -0.5f));
}

static void speed_tune_show(void)
{
    speed_control_state_t state;

    speed_control_get_state(&state);
    OLED_ShowString(0U, 0U, (uint8 *)"SPEED PI TEST", 8U);
    OLED_ShowString(0U, 1U, (uint8 *)"UART3 115200", 8U);
    OLED_ShowString(0U, 2U, (uint8 *)"TARGET:", 8U);
    OLED_ShowNum(42U, 2U, speed_tune_targets[speed_tune_target_index], 3U, 8U);
    OLED_ShowString(0U, 3U, (uint8 *)"L:", 8U);
    OLED_ShowNum(12U, 3U,
                 (uint32)((state.left_speed_mm_s > 0.0f) ?
                          state.left_speed_mm_s : 0.0f), 4U, 8U);
    OLED_ShowString(48U, 3U, (uint8 *)"PWM:", 8U);
    OLED_ShowNum(72U, 3U,
                 (uint32)((state.left_duty > 0) ? state.left_duty : 0), 4U, 8U);
    OLED_ShowString(0U, 4U, (uint8 *)"R:", 8U);
    OLED_ShowNum(12U, 4U,
                 (uint32)((state.right_speed_mm_s > 0.0f) ?
                          state.right_speed_mm_s : 0.0f), 4U, 8U);
    OLED_ShowString(48U, 4U, (uint8 *)"PWM:", 8U);
    OLED_ShowNum(72U, 4U,
                 (uint32)((state.right_duty > 0) ? state.right_duty : 0), 4U, 8U);
    OLED_ShowString(0U, 6U, (uint8 *)"MODE:TARGET", 8U);
    OLED_ShowString(0U, 7U, (uint8 *)(speed_tune_running ?
                                     "RUN OK:STOP" : "STOP OK:RUN"), 8U);
}

static uint16 speed_tune_abs_duty(int16 duty)
{
    return (uint16)((duty < 0) ? -duty : duty);
}

static uint16 speed_tune_positive_speed(float speed)
{
    return (uint16)((speed > 0.0f) ? speed_tune_round(speed) : 0);
}

static void speed_tune_collect_result(uint32 elapsed_ms,
                                      const speed_control_state_t *state)
{
    uint16 left_duty = speed_tune_abs_duty(state->left_duty);
    uint16 right_duty = speed_tune_abs_duty(state->right_duty);

    if (left_duty > speed_tune_left_peak_duty)
    {
        speed_tune_left_peak_duty = left_duty;
    }
    if (right_duty > speed_tune_right_peak_duty)
    {
        speed_tune_right_peak_duty = right_duty;
    }
    if (elapsed_ms >= SPEED_TUNE_STABLE_MS)
    {
        speed_tune_left_sum += speed_tune_positive_speed(state->left_speed_mm_s);
        speed_tune_right_sum += speed_tune_positive_speed(state->right_speed_mm_s);
        speed_tune_sample_count++;
    }
}

static void speed_tune_show_result(const char *reason)
{
    OLED_Clear();
    OLED_ShowString(0U, 0U, (uint8 *)"SPEED PI RESULT", 8U);
    OLED_ShowString(0U, 1U, (uint8 *)"TARGET:", 8U);
    OLED_ShowNum(42U, 1U, speed_tune_targets[speed_tune_target_index], 3U, 8U);
    OLED_ShowString(0U, 2U, (uint8 *)"AVG L:", 8U);
    OLED_ShowNum(36U, 2U, speed_tune_left_average, 4U, 8U);
    OLED_ShowString(0U, 3U, (uint8 *)"AVG R:", 8U);
    OLED_ShowNum(36U, 3U, speed_tune_right_average, 4U, 8U);
    OLED_ShowString(0U, 4U, (uint8 *)"MAXP L:", 8U);
    OLED_ShowNum(42U, 4U, speed_tune_left_peak_duty, 4U, 8U);
    OLED_ShowString(0U, 5U, (uint8 *)"MAXP R:", 8U);
    OLED_ShowNum(42U, 5U, speed_tune_right_peak_duty, 4U, 8U);
    OLED_ShowString(0U, 6U, (uint8 *)reason, 8U);
    OLED_ShowString(0U, 7U, (uint8 *)"MODE:SEL OK:RUN", 8U);
}

static void speed_tune_send_header(void)
{
    Serial_SendString(
        "ms,target,l_set,l_raw,l_filt,l_pwm,r_set,r_raw,r_filt,r_pwm\r\n");
}

static void speed_tune_send_row(uint32 elapsed_ms)
{
    speed_control_state_t state;

    speed_control_get_state(&state);
    Serial_Printf("%lu,%u,%ld,%ld,%ld,%d,%ld,%ld,%ld,%d\r\n",
                  (unsigned long)elapsed_ms,
                  speed_tune_targets[speed_tune_target_index],
                  (long)speed_tune_round(state.left_setpoint_mm_s),
                  (long)speed_tune_round(state.left_raw_mm_s),
                  (long)speed_tune_round(state.left_speed_mm_s),
                  state.left_duty,
                  (long)speed_tune_round(state.right_setpoint_mm_s),
                  (long)speed_tune_round(state.right_raw_mm_s),
                  (long)speed_tune_round(state.right_speed_mm_s),
                  state.right_duty);
}

static void speed_tune_start(void)
{
    float target = (float)speed_tune_targets[speed_tune_target_index];

    speed_control_stop();
    speed_control_set_acceleration(CAR_LEFT_SPEED_ACCEL_MM_S2,
                                   CAR_RIGHT_SPEED_ACCEL_MM_S2);
    speed_control_set_target(target, target);
    speed_tune_running = 1U;
    speed_tune_show();
    speed_control_start();
    speed_tune_start_ms = system_get_ms();
    speed_tune_telemetry_ms = speed_tune_start_ms;
    speed_tune_left_sum = 0U;
    speed_tune_right_sum = 0U;
    speed_tune_sample_count = 0U;
    speed_tune_left_peak_duty = 0U;
    speed_tune_right_peak_duty = 0U;
    speed_tune_left_average = 0U;
    speed_tune_right_average = 0U;
    Serial_Printf("START,target=%u,left_accel=%u,right_accel=%u\r\n",
                  speed_tune_targets[speed_tune_target_index],
                  (uint16)CAR_LEFT_SPEED_ACCEL_MM_S2,
                  (uint16)CAR_RIGHT_SPEED_ACCEL_MM_S2);
    speed_tune_send_header();
}

static void speed_tune_stop(const char *reason)
{
    if (speed_tune_sample_count > 0U)
    {
        speed_tune_left_average =
            (uint16)(speed_tune_left_sum / speed_tune_sample_count);
        speed_tune_right_average =
            (uint16)(speed_tune_right_sum / speed_tune_sample_count);
    }
    speed_control_stop();
    speed_tune_running = 0U;
    Serial_Printf("STOP,%s\r\n", reason);
    speed_tune_show_result(reason);
}

void speed_tune_init(void)
{
    speed_control_stop();
    speed_tune_target_index = 0U;
    speed_tune_running = 0U;
    speed_tune_start_ms = system_get_ms();
    speed_tune_telemetry_ms = speed_tune_start_ms;
    speed_tune_display_ms = speed_tune_start_ms;
    OLED_Clear();
    speed_tune_show();
    Serial_SendString("MSPM0 SPEED PI TEST READY,115200 8N1\r\n");
}

void speed_tune_loop(void)
{
    uint32 now_ms = system_get_ms();
    uint32 elapsed_ms;
    uint8 key = Key_GetNum();

    if (!speed_tune_running && (key == 1U))
    {
        speed_tune_target_index =
            (uint8)((speed_tune_target_index + 1U) % SPEED_TUNE_TARGET_COUNT);
        speed_tune_sample_count = 0U;
        speed_tune_show();
    }
    if (key == 4U)
    {
        if (speed_tune_running)
        {
            speed_tune_stop("KEY");
        }
        else
        {
            speed_tune_start();
        }
    }

    /* Starting a test can advance the system tick. Refresh it before
       subtracting the newly recorded start time to avoid uint32 underflow. */
    now_ms = system_get_ms();

    if (speed_tune_running)
    {
        elapsed_ms = (uint32)(now_ms - speed_tune_start_ms);
        if ((uint32)(now_ms - speed_tune_telemetry_ms) >= SPEED_TUNE_TELEMETRY_MS)
        {
            speed_control_state_t state;

            speed_tune_telemetry_ms = now_ms;
            speed_control_get_state(&state);
            speed_tune_collect_result(elapsed_ms, &state);
            speed_tune_send_row(elapsed_ms);
        }
        if (elapsed_ms >= SPEED_TUNE_TEST_MS)
        {
            speed_tune_stop("DONE");
        }
    }

    /* A full software-I2C OLED redraw blocks long enough to disturb the
       20 ms speed loop. Keep the display static while collecting data. */
    if (!speed_tune_running && (speed_tune_sample_count == 0U) &&
        ((uint32)(now_ms - speed_tune_display_ms) >= SPEED_TUNE_DISPLAY_MS))
    {
        speed_tune_display_ms = now_ms;
        speed_tune_show();
    }
}

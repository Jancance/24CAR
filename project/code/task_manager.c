#include "task_manager.h"
#include "board_pins.h"

#include <stddef.h>

#define TASK1_SPEED_MM_S                 (200.0f)
#define TASK1_DISTANCE_GATE_MM           (700.0f)
#define TASK1_TIMEOUT_MS                 (12000U)
#define TASK1_MAX_DISTANCE_MM            (1400.0f)
#define TASK1_MIN_BLACK_COUNT            (2U)
#define TASK1_LINE_CONFIRM_COUNT         (3U)
#define TASK1_SAMPLE_PERIOD_MS           (10U)
#define TASK2_SPEED_MM_S                 (200.0f)
#define TASK2_STRAIGHT_GATE_MM           (700.0f)
#define TASK2_STRAIGHT_MAX_MM            (1400.0f)
#define TASK2_ARC_MIN_MM                 (900.0f)
#define TASK2_ARC_MAX_MM                 (1600.0f)
#define TASK2_TOTAL_TIMEOUT_MS           (30000U)
#define TASK2_LINE_FOUND_CONFIRM_COUNT   (3U)
#define TASK2_LINE_LOST_CONFIRM_COUNT    (8U)
#define TASK2_POINT_SIGNAL_MS            (100U)
#define TASK_MANAGER_DISPLAY_PERIOD_MS   (500U)
#define TASK_MANAGER_CAL_DISPLAY_PERIOD_MS (1000U)

static task_manager_state_t task_state;
static uint8 selected_task;
static uint32 task_start_ms;
static uint32 display_ms;
static uint8 b_line_confirm_count;
static float task_distance_mm;
static uint8 task_line_count;
static uint8 task1_gray_enabled;
static uint8 task1_last_reported_line_count;
static uint32 task_sample_ms;
static uint8 task2_line_event_count;
static uint32 task_signal_start_ms;
static uint8 task_signal_active;

static float task_manager_average_distance(void)
{
    float left_mm;
    float right_mm;

    car_encoder_get_distance_mm(&left_mm, &right_mm);
    if (left_mm < 0.0f)
    {
        left_mm = -left_mm;
    }
    if (right_mm < 0.0f)
    {
        right_mm = -right_mm;
    }
    return (left_mm + right_mm) * 0.5f;
}

static uint8 task_manager_valid_black_count(void)
{
    uint8 index;
    uint8 count = 0U;

    gray_update();
    for (index = 0U; index < CAR_GRAY_COUNT; index++)
    {
        count += gray_values[index];
    }
    return count;
}

static uint8 task_manager_mask_count(uint8 mask)
{
    uint8 count = 0U;

    while (mask != 0U)
    {
        count += (uint8)(mask & 1U);
        mask >>= 1U;
    }
    return count;
}

static void task_manager_stop_motion(void)
{
    line_follow_stop();
    position_control_stop();
}

static void task_manager_signal_point(void)
{
    task_signal_start_ms = system_get_ms();
    task_signal_active = 1U;
    led_set(0U, 1U);
    buzzer_beep_start(TASK2_POINT_SIGNAL_MS);
}

static void task_manager_signal_loop(void)
{
    if (task_signal_active &&
        (uint32)(system_get_ms() - task_signal_start_ms) >=
            TASK2_POINT_SIGNAL_MS)
    {
        task_signal_active = 0U;
        led_set(0U, 0U);
    }
}

static void task_manager_fail(void)
{
    task_manager_state_t failed_state = task_state;

    task_manager_stop_motion();
    task_state = TASK_MANAGER_ERROR;
    buzzer_beep_start(150U);
    Serial_Printf("TASK,ERROR,STATE=%u,DIST=%.1f,LINE=%u\r\n",
                  failed_state, task_distance_mm, task_line_count);
}

static void task_manager_show_status(void)
{
    const char *state_text;
    position_control_state_t position_state;

    position_control_get_state(&position_state);

    if (task_state == TASK_MANAGER_TASK1_RUN)
    {
        state_text = "T1 RUN    ";
    }
    else if (task_state == TASK_MANAGER_TASK1_DONE)
    {
        state_text = "T1 DONE   ";
    }
    else if (task_state == TASK_MANAGER_TASK2_AB)
    {
        state_text = "T2 A-B    ";
    }
    else if (task_state == TASK_MANAGER_TASK2_BC)
    {
        state_text = "T2 B-C ARC";
    }
    else if (task_state == TASK_MANAGER_TASK2_CD)
    {
        state_text = "T2 C-D    ";
    }
    else if (task_state == TASK_MANAGER_TASK2_DA)
    {
        state_text = "T2 D-A ARC";
    }
    else if (task_state == TASK_MANAGER_TASK2_DONE)
    {
        state_text = "T2 DONE   ";
    }
    else if (task_state == TASK_MANAGER_ERROR)
    {
        state_text = "TASK ERROR";
    }
    else if (line_follow_is_active())
    {
        state_text = "LINE RUN  ";
    }
    else
    {
        state_text = position_state.icm_calibrated ?
                     "READY     " : "WAIT IMU  ";
    }

    OLED_ShowString(0U, 0U, (uint8 *)"TASK:", 8U);
    OLED_ShowNum(30U, 0U, selected_task, 1U, 8U);
    OLED_ShowString(0U, 1U, (uint8 *)state_text, 8U);
    OLED_ShowString(0U, 2U, (uint8 *)"IMU:", 8U);
    OLED_ShowString(30U, 2U,
                    (uint8 *)(position_state.icm_calibrated ? "OK  " : "CAL "),
                    8U);
    OLED_ShowString(0U, 3U, (uint8 *)"DIST:", 8U);
    OLED_ShowNum(30U, 3U,
                 (uint32)((task_distance_mm < 0.0f) ? 0.0f :
                           task_distance_mm + 0.5f), 4U, 8U);
    OLED_ShowString(60U, 3U, (uint8 *)"mm", 8U);
    OLED_ShowString(0U, 4U, (uint8 *)"LINE:", 8U);
    OLED_ShowNum(30U, 4U, task_line_count, 1U, 8U);
}

static uint8 task_manager_start_straight(task_manager_state_t next_state)
{
    line_follow_stop();
    car_encoder_clear();
    task_distance_mm = 0.0f;
    task_line_count = 0U;
    task2_line_event_count = 0U;
    if (!position_control_start(TASK2_SPEED_MM_S))
    {
        return 0U;
    }
    task_state = next_state;
    return 1U;
}

static uint8 task_manager_start_arc(task_manager_state_t next_state)
{
    position_control_stop();
    car_encoder_clear();
    task_distance_mm = 0.0f;
    task_line_count = 0U;
    task2_line_event_count = 0U;
    if (!line_follow_start(TASK2_SPEED_MM_S))
    {
        return 0U;
    }
    task_state = next_state;
    return 1U;
}

static void task_manager_start_selected(void)
{
    position_control_state_t position_state;

    position_control_get_state(&position_state);
    if (!position_state.icm_calibrated)
    {
        buzzer_beep_start(80U);
        return;
    }

    task_manager_stop_motion();
    b_line_confirm_count = 0U;
    task2_line_event_count = 0U;
    task_start_ms = system_get_ms();
    task_sample_ms = task_start_ms;
    led_set(0U, 0U);
    led_set(1U, 0U);

    if (selected_task == 1U)
    {
        car_encoder_clear();
        task_distance_mm = 0.0f;
        task_line_count = 0U;
        task1_gray_enabled = 0U;
        task1_last_reported_line_count = 0xFFU;
        if (position_control_start(TASK1_SPEED_MM_S))
        {
            task_state = TASK_MANAGER_TASK1_RUN;
            Serial_SendString("TASK,1,START\r\n");
        }
        else
        {
            task_manager_fail();
        }
    }
    else if (selected_task == 2U)
    {
        if (!task_manager_start_straight(TASK_MANAGER_TASK2_AB))
        {
            task_manager_fail();
        }
    }
    else
    {
        task_manager_fail();
    }
}

static void task_manager_stop_at_b(void)
{
    position_control_stop();
    task_state = TASK_MANAGER_TASK1_DONE;
    led_set(0U, 1U);
    buzzer_beep_start(120U);
    Serial_Printf("TASK,1,DONE,DIST=%.1f,LINE=%u\r\n",
                  task_distance_mm, task_line_count);
}

static void task_manager_update_task1(void)
{
    uint8 black_count;
    uint32 now_ms = system_get_ms();

    if ((uint32)(now_ms - task_sample_ms) < TASK1_SAMPLE_PERIOD_MS)
    {
        return;
    }
    task_sample_ms = now_ms;

    task_distance_mm = task_manager_average_distance();
    if ((uint32)(now_ms - task_start_ms) > TASK1_TIMEOUT_MS ||
        task_distance_mm > TASK1_MAX_DISTANCE_MM)
    {
        task_manager_fail();
        return;
    }

    if (task_distance_mm < TASK1_DISTANCE_GATE_MM)
    {
        b_line_confirm_count = 0U;
        task_line_count = 0U;
        return;
    }

    if (!task1_gray_enabled)
    {
        task1_gray_enabled = 1U;
        Serial_Printf("TASK,1,GRAY_ON,DIST=%.1f\r\n", task_distance_mm);
    }

    black_count = task_manager_valid_black_count();
    task_line_count = black_count;
    if (black_count != task1_last_reported_line_count)
    {
        task1_last_reported_line_count = black_count;
        Serial_Printf("TASK,1,GRAY,DIST=%.1f,LINE=%u\r\n",
                      task_distance_mm, black_count);
    }
    if (black_count >= TASK1_MIN_BLACK_COUNT)
    {
        if (b_line_confirm_count < 255U)
        {
            b_line_confirm_count++;
        }
    }
    else
    {
        b_line_confirm_count = 0U;
    }

    if (b_line_confirm_count >= TASK1_LINE_CONFIRM_COUNT)
    {
        task_manager_stop_at_b();
    }
}

static void task_manager_task2_reach_b(void)
{
    task_manager_signal_point();
    if (!task_manager_start_arc(TASK_MANAGER_TASK2_BC))
    {
        task_manager_fail();
    }
}

static void task_manager_task2_reach_c(void)
{
    task_manager_signal_point();
    if (!task_manager_start_straight(TASK_MANAGER_TASK2_CD))
    {
        task_manager_fail();
    }
}

static void task_manager_task2_reach_d(void)
{
    task_manager_signal_point();
    if (!task_manager_start_arc(TASK_MANAGER_TASK2_DA))
    {
        task_manager_fail();
    }
}

static void task_manager_task2_reach_a(void)
{
    task_manager_stop_motion();
    task_manager_signal_point();
    task_state = TASK_MANAGER_TASK2_DONE;
    led_set(1U, 1U);
}

static void task_manager_update_task2_straight(uint8 reach_b)
{
    uint8 black_count;

    if (task_distance_mm > TASK2_STRAIGHT_MAX_MM)
    {
        task_manager_fail();
        return;
    }
    if (task_distance_mm < TASK2_STRAIGHT_GATE_MM)
    {
        task2_line_event_count = 0U;
        task_line_count = 0U;
        return;
    }

    black_count = task_manager_valid_black_count();
    task_line_count = black_count;
    if (black_count >= TASK1_MIN_BLACK_COUNT)
    {
        if (task2_line_event_count < 255U)
        {
            task2_line_event_count++;
        }
    }
    else
    {
        task2_line_event_count = 0U;
    }

    if (task2_line_event_count >= TASK2_LINE_FOUND_CONFIRM_COUNT)
    {
        if (reach_b)
        {
            task_manager_task2_reach_b();
        }
        else
        {
            task_manager_task2_reach_d();
        }
    }
}

static void task_manager_update_task2_arc(uint8 reach_c)
{
    float error;
    uint8 mask;
    uint8 found;

    if (task_distance_mm > TASK2_ARC_MAX_MM)
    {
        task_manager_fail();
        return;
    }

    found = line_follow_sample_now(&error, &mask);
    (void)error;
    task_line_count = task_manager_mask_count(mask);
    if (task_distance_mm < TASK2_ARC_MIN_MM)
    {
        task2_line_event_count = 0U;
        return;
    }

    if (!found)
    {
        if (task2_line_event_count < 255U)
        {
            task2_line_event_count++;
        }
    }
    else
    {
        task2_line_event_count = 0U;
    }

    if (task2_line_event_count >= TASK2_LINE_LOST_CONFIRM_COUNT)
    {
        if (reach_c)
        {
            task_manager_task2_reach_c();
        }
        else
        {
            task_manager_task2_reach_a();
        }
    }
}

static void task_manager_update_task2(void)
{
    uint32 now_ms = system_get_ms();

    if ((uint32)(now_ms - task_start_ms) > TASK2_TOTAL_TIMEOUT_MS)
    {
        task_manager_fail();
        return;
    }
    if ((uint32)(now_ms - task_sample_ms) < TASK1_SAMPLE_PERIOD_MS)
    {
        return;
    }
    task_sample_ms = now_ms;
    task_distance_mm = task_manager_average_distance();

    if (task_state == TASK_MANAGER_TASK2_AB)
    {
        task_manager_update_task2_straight(1U);
    }
    else if (task_state == TASK_MANAGER_TASK2_BC)
    {
        task_manager_update_task2_arc(1U);
    }
    else if (task_state == TASK_MANAGER_TASK2_CD)
    {
        task_manager_update_task2_straight(0U);
    }
    else if (task_state == TASK_MANAGER_TASK2_DA)
    {
        task_manager_update_task2_arc(0U);
    }
}

void task_manager_init(void)
{
    task_state = TASK_MANAGER_IDLE;
    selected_task = 1U;
    task_start_ms = system_get_ms();
    task_sample_ms = task_start_ms;
    display_ms = task_start_ms;
    b_line_confirm_count = 0U;
    task2_line_event_count = 0U;
    task_signal_start_ms = task_start_ms;
    task_signal_active = 0U;
    task_distance_mm = 0.0f;
    task_line_count = 0U;
    task1_gray_enabled = 0U;
    task1_last_reported_line_count = 0xFFU;
    led_set(0U, 0U);
    led_set(1U, 0U);
    OLED_Clear();
    task_manager_show_status();
}

void key_Loop(void)
{
    uint8 key = Key_GetNum();

    task_manager_signal_loop();

    if (task_manager_is_running())
    {
        if (key == 4U)
        {
            task_manager_abort();
            return;
        }
        if (task_state == TASK_MANAGER_TASK1_RUN)
        {
            task_manager_update_task1();
        }
        else
        {
            task_manager_update_task2();
        }
        return;
    }

    if (key == 1U)
    {
        selected_task++;
        if (selected_task > 4U)
        {
            selected_task = 1U;
        }
    }
    else if (key == 2U)
    {
        selected_task++;
        if (selected_task > 4U)
        {
            selected_task = 1U;
        }
    }
    else if (key == 3U)
    {
        selected_task = (selected_task <= 1U) ? 4U :
                        (uint8)(selected_task - 1U);
    }
    else if (key == 4U)
    {
        if (task_state == TASK_MANAGER_TASK1_DONE ||
            task_state == TASK_MANAGER_TASK2_DONE ||
            task_state == TASK_MANAGER_ERROR)
        {
            task_state = TASK_MANAGER_IDLE;
            led_set(0U, 0U);
            led_set(1U, 0U);
        }
        else
        {
            task_manager_start_selected();
        }
    }
}

void task_manager_abort(void)
{
    task_manager_stop_motion();
    task_state = TASK_MANAGER_IDLE;
    task_signal_active = 0U;
    led_set(0U, 0U);
    led_set(1U, 0U);
}

void task_manager_display_loop(void)
{
    position_control_state_t position_state;
    speed_control_state_t speed_state;
    uint32 display_period_ms;
    uint32 now_ms = system_get_ms();

    speed_control_get_state(&speed_state);
    if (speed_state.running)
    {
        return;
    }
    position_control_get_state(&position_state);
    display_period_ms = position_state.icm_calibrated ?
                        TASK_MANAGER_DISPLAY_PERIOD_MS :
                        TASK_MANAGER_CAL_DISPLAY_PERIOD_MS;
    if ((uint32)(now_ms - display_ms) < display_period_ms)
    {
        return;
    }
    display_ms = now_ms;
    task_manager_show_status();
}

uint8 task_manager_is_running(void)
{
    return ((task_state == TASK_MANAGER_TASK1_RUN) ||
            (task_state == TASK_MANAGER_TASK2_AB) ||
            (task_state == TASK_MANAGER_TASK2_BC) ||
            (task_state == TASK_MANAGER_TASK2_CD) ||
            (task_state == TASK_MANAGER_TASK2_DA)) ? 1U : 0U;
}

task_manager_state_t task_manager_get_state(void)
{
    return task_state;
}

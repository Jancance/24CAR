#ifndef __TASK_MANAGER_H
#define __TASK_MANAGER_H

#include "zf_common_headfile.h"

typedef enum
{
    TASK_MANAGER_IDLE = 0,
    TASK_MANAGER_TASK1_RUN,
    TASK_MANAGER_TASK1_DONE,
    TASK_MANAGER_TASK2_AB,
    TASK_MANAGER_TASK2_BC,
    TASK_MANAGER_TASK2_CD,
    TASK_MANAGER_TASK2_DA,
    TASK_MANAGER_TASK2_DONE,
    TASK_MANAGER_ERROR
} task_manager_state_t;

void task_manager_init(void);
void key_Loop(void);
void task_manager_abort(void);
void task_manager_display_loop(void);
uint8 task_manager_is_running(void);
task_manager_state_t task_manager_get_state(void);

#endif

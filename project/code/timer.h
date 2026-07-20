#ifndef __TIMER_H
#define __TIMER_H

#include "zf_common_headfile.h"

// TIMG12 is reserved for the 1 ms system tick. TIMG0 belongs to motor PWM.
#define CAR_SYSTEM_PIT      (PIT_TIM_G12)
#define CAR_SYSTEM_PIT_IRQn (TIMG12_INT_IRQn)

void system_pit_init(void);
uint32 system_get_ms(void);

#endif

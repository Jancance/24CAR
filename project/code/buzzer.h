#ifndef __BUZZER_H
#define __BUZZER_H

#include "zf_common_headfile.h"

void buzzer_init(void);
void buzzer_on(void);
void buzzer_off(void);
void buzzer_toggle(void);
void buzzer_beep(uint16 ms);
void buzzer_beep_start(uint16 ms);
void buzzer_loop(void);







#endif

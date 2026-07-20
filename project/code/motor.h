#ifndef __MOTOR_H
#define __MOTOR_H

#include "zf_common_headfile.h"

void motor_init(void);
void motor_set(int16 left, int16 right);
void motor_stop(void);

#endif

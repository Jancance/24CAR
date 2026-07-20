#ifndef __ENCODER_H
#define __ENCODER_H

#include "zf_common_headfile.h"

void car_encoder_init(void);
void car_encoder_clear(void);
void car_encoder_get_count(int32 *left_count, int32 *right_count);
void car_encoder_get_delta(int32 *left_delta, int32 *right_delta);

#endif

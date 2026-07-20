#ifndef __GRAY_H
#define __GRAY_H

#include "zf_common_headfile.h"

// Index 0..7 corresponds to grayscale channels 1..8.
// A value of 1 means the channel sees the configured active surface (black).
extern uint8 gray_values[8];

void gray_init(void);
void gray_update(void);
uint8 gray_read_channel(uint8 index);
uint8 gray_read_raw(void);
uint8 gray_read_active(void);

#endif

#ifndef __GRAY_H
#define __GRAY_H

#include "zf_common_headfile.h"

void gray_init(void);
uint8 gray_read_channel(uint8 index);
uint8 gray_read_raw(void);
uint8 gray_read_active(void);

#endif

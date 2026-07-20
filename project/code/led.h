#ifndef __LED_H
#define __LED_H

#include "zf_common_headfile.h"

void led_init(void);
// 第二版扩展板上的两颗高电平有效状态 LED，index 为 0 或 1。
void led_set(uint8 index, uint8 state);
void led_toggle(uint8 index);
// 主控板原有三色 RGB，低电平有效，index 为 0~2。
void rgb_led_set(uint8 index, uint8 state);
void rgb_led_toggle(uint8 index);






#endif

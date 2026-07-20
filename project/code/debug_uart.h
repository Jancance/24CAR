#ifndef __DEBUG_UART_H
#define __DEBUG_UART_H

#include "zf_common_headfile.h"

void debug_uart_init(uint32 baud);
void debug_uart_write_string(const char *str);
void debug_uart_write_u32(uint32 value);

#endif

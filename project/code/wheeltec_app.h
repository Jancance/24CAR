#ifndef __WHEELTEC_APP_H
#define __WHEELTEC_APP_H

#include "zf_common_headfile.h"

void wheeltec_app_init(void);
uint8 wheeltec_app_process_byte(uint8 data);
uint8 wheeltec_app_is_stream_enabled(void);
void wheeltec_app_toggle_line_follow(void);
void wheeltec_app_loop(void);

#endif

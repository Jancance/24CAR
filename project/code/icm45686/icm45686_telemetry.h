#ifndef __ICM45686_TELEMETRY_H
#define __ICM45686_TELEMETRY_H

#include "icm45686.h"

// 通过第二版 UART3(PA26/PA25)输出一帧ASCII遥测数据。
void icm45686_telemetry_send(uint32 timestamp_ms);

#endif

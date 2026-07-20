#ifndef __SERIAL_H
#define __SERIAL_H

#include "zf_common_headfile.h"

void Serial_Init(void);
void Serial_SendByte(uint8 byte);
void Serial_SendArray(const uint8 *array, uint16 length);
void Serial_SendString(const char *string);
void Serial_SendNumber(uint32 number, uint8 length);
void Serial_Printf(const char *format, ...);

// Compatible with the reference usage: reading the flag clears it.
uint8 Serial_GetRxFlag(void);
uint8 Serial_GetRxData(void);
uint8 Serial_GetRxOverflow(void);

#endif

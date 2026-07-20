#ifndef __ICM45686_PORT_H
#define __ICM45686_PORT_H

#include "zf_common_headfile.h"

void icm45686_port_init(void);
int icm45686_port_read(uint8_t reg, uint8_t *data, uint32_t len);
int icm45686_port_write(uint8_t reg, const uint8_t *data, uint32_t len);
void icm45686_port_delay_us(uint32_t us);
uint32 icm45686_port_get_ms(void);
uint8 icm45686_port_int1_level(void);

#endif

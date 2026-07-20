#ifndef __ZDT_PORT_H
#define __ZDT_PORT_H

#include "zf_common_headfile.h"

typedef enum
{
    ZDT_PORT_YAW = 0,
    ZDT_PORT_PITCH = 1,
    ZDT_PORT_COUNT
} zdt_port_enum;

// Hardware adaptation for the two independent TTL UART links.
void zdt_port_init(zdt_port_enum port);
void zdt_port_write(zdt_port_enum port, const uint8 *data, uint32 length);
uint8 zdt_port_read(zdt_port_enum port, uint8 *data);

#endif

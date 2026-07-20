#include "zdt_port.h"
#include "board_pins.h"

static uart_index_enum zdt_port_uart(zdt_port_enum port)
{
    return (port == ZDT_PORT_PITCH) ? CAR_GIMBAL_PITCH_UART : CAR_GIMBAL_YAW_UART;
}

void zdt_port_init(zdt_port_enum port)
{
    if (port == ZDT_PORT_PITCH)
    {
        uart_init(CAR_GIMBAL_PITCH_UART,
                  CAR_GIMBAL_UART_BAUD,
                  CAR_GIMBAL_PITCH_UART_TX_PIN,
                  CAR_GIMBAL_PITCH_UART_RX_PIN);
    }
    else
    {
        uart_init(CAR_GIMBAL_YAW_UART,
                  CAR_GIMBAL_UART_BAUD,
                  CAR_GIMBAL_YAW_UART_TX_PIN,
                  CAR_GIMBAL_YAW_UART_RX_PIN);
    }
}

void zdt_port_write(zdt_port_enum port, const uint8 *data, uint32 length)
{
    if ((data == NULL) || (length == 0U) || (port >= ZDT_PORT_COUNT))
    {
        return;
    }

    uart_write_buffer(zdt_port_uart(port), data, length);
}

uint8 zdt_port_read(zdt_port_enum port, uint8 *data)
{
    if ((data == NULL) || (port >= ZDT_PORT_COUNT))
    {
        return 0U;
    }

    return uart_query_byte(zdt_port_uart(port), data);
}

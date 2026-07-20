#include "debug_uart.h"
#include "board_pins.h"

#include <stddef.h>

void debug_uart_init(uint32 baud)
{
    uart_init(CAR_DEBUG_UART, baud, CAR_DEBUG_UART_TX_PIN, CAR_DEBUG_UART_RX_PIN);
}

void debug_uart_write_string(const char *str)
{
    if (str == NULL)
    {
        return;
    }

    uart_write_string(CAR_DEBUG_UART, str);
}

void debug_uart_write_u32(uint32 value)
{
    char text[11];
    uint8 index = 0;
    uint8 write_index;

    if (value == 0U)
    {
        uart_write_byte(CAR_DEBUG_UART, '0');
        return;
    }

    while ((value > 0U) && (index < sizeof(text)))
    {
        text[index++] = (char)('0' + (value % 10U));
        value /= 10U;
    }

    while (index > 0U)
    {
        write_index = (uint8)(index - 1U);
        uart_write_byte(CAR_DEBUG_UART, (uint8)text[write_index]);
        index--;
    }
}

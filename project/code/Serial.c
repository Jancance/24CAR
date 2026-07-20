#include "Serial.h"
#include "board_pins.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

#define SERIAL_PRINTF_BUFFER_SIZE (128U)
#define SERIAL_RX_BUFFER_SIZE     (64U)

static uint8 serial_rx_data;
static volatile uint8 serial_rx_buffer[SERIAL_RX_BUFFER_SIZE];
static volatile uint8 serial_rx_write_index;
static volatile uint8 serial_rx_read_index;
static volatile uint8 serial_rx_overflow_flag;

static void serial_uart_callback(uint32 state, void *ptr)
{
    uint8 data;
    uint8 next_index;

    (void)ptr;
    if (state != UART_INTERRUPT_STATE_RX)
    {
        return;
    }

    /* The interrupt only stores bytes. Parsing and display stay in main(). */
    while (uart_query_byte(CAR_DEBUG_UART, &data))
    {
        next_index = (uint8)((serial_rx_write_index + 1U) % SERIAL_RX_BUFFER_SIZE);
        if (next_index == serial_rx_read_index)
        {
            serial_rx_overflow_flag = 1U;
            break;
        }
        serial_rx_buffer[serial_rx_write_index] = data;
        serial_rx_write_index = next_index;
    }
}

static uint8 serial_read_byte(uint8 *data)
{
    uint32 primask;

    if (data == NULL)
    {
        return 0U;
    }

    primask = interrupt_global_disable();
    if (serial_rx_read_index == serial_rx_write_index)
    {
        interrupt_global_enable(primask);
        return 0U;
    }

    *data = serial_rx_buffer[serial_rx_read_index];
    serial_rx_read_index = (uint8)((serial_rx_read_index + 1U) % SERIAL_RX_BUFFER_SIZE);
    interrupt_global_enable(primask);
    return 1U;
}

void Serial_Init(void)
{
    serial_rx_data = 0U;
    serial_rx_write_index = 0U;
    serial_rx_read_index = 0U;
    serial_rx_overflow_flag = 0U;

    uart_init(CAR_DEBUG_UART, CAR_DEBUG_UART_BAUD,
              CAR_DEBUG_UART_TX_PIN, CAR_DEBUG_UART_RX_PIN);
    uart_set_callback(CAR_DEBUG_UART, serial_uart_callback, NULL);
    uart_set_interrupt_config(CAR_DEBUG_UART, UART_INTERRUPT_CONFIG_RX_ENABLE);
}

void Serial_SendByte(uint8 byte)
{
    uart_write_byte(CAR_DEBUG_UART, byte);
}

void Serial_SendArray(const uint8 *array, uint16 length)
{
    uint16 index;

    if (array == NULL)
    {
        return;
    }

    for (index = 0U; index < length; index++)
    {
        Serial_SendByte(array[index]);
    }
}

void Serial_SendString(const char *string)
{
    if (string == NULL)
    {
        return;
    }

    uart_write_string(CAR_DEBUG_UART, string);
}

void Serial_SendNumber(uint32 number, uint8 length)
{
    uint8 index;
    uint32 divisor = 1U;

    if (length > 10U)
    {
        length = 10U;
    }
    if (length == 0U)
    {
        return;
    }

    for (index = 1U; index < length; index++)
    {
        divisor *= 10U;
    }

    for (index = 0U; index < length; index++)
    {
        Serial_SendByte((uint8)('0' + (number / divisor) % 10U));
        divisor = (divisor > 1U) ? (divisor / 10U) : 1U;
    }
}

void Serial_Printf(const char *format, ...)
{
    char text[SERIAL_PRINTF_BUFFER_SIZE];
    int length;
    va_list arguments;

    if (format == NULL)
    {
        return;
    }

    va_start(arguments, format);
    length = vsnprintf(text, sizeof(text), format, arguments);
    va_end(arguments);

    if (length > 0)
    {
        Serial_SendString(text);
    }
}

uint8 Serial_GetRxFlag(void)
{
    /* Reading the flag consumes one queued byte, like clearing an RX flag. */
    return serial_read_byte(&serial_rx_data);
}

uint8 Serial_GetRxData(void)
{
    return serial_rx_data;
}

uint8 Serial_GetRxOverflow(void)
{
    uint8 overflow;
    uint32 primask = interrupt_global_disable();

    overflow = serial_rx_overflow_flag;
    serial_rx_overflow_flag = 0U;
    interrupt_global_enable(primask);
    return overflow;
}

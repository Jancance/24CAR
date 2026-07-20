#include "icm45686_port.h"
#include "board_pins.h"

static uint8 icm45686_spi_exchange(uint8 value)
{
    uint8 received = 0U;

    spi_transfer_8bit(CAR_ICM45686_SPI, &value, &received, 1U);
    return received;
}

void icm45686_port_init(void)
{
    gpio_init(CAR_ICM45686_CS_PIN, GPO, GPIO_HIGH, GPO_PUSH_PULL);
    gpio_init(CAR_ICM45686_INT1_PIN, GPI, GPIO_LOW, GPI_PULL_DOWN);

    spi_init(CAR_ICM45686_SPI,
             SPI_MODE3,
             CAR_ICM45686_SPI_BAUD,
             CAR_ICM45686_SCK_PIN,
             CAR_ICM45686_MOSI_PIN,
             CAR_ICM45686_MISO_PIN,
             SPI_CS_NULL);
}

int icm45686_port_read(uint8_t reg, uint8_t *data, uint32_t len)
{
    uint32_t index;

    if ((data == NULL) && (len != 0U))
    {
        return -1;
    }

    gpio_low(CAR_ICM45686_CS_PIN);
    (void)icm45686_spi_exchange((uint8)(reg | 0x80U));
    for (index = 0U; index < len; index++)
    {
        data[index] = icm45686_spi_exchange(0x00U);
    }
    gpio_high(CAR_ICM45686_CS_PIN);

    return 0;
}

int icm45686_port_write(uint8_t reg, const uint8_t *data, uint32_t len)
{
    uint32_t index;

    if ((data == NULL) && (len != 0U))
    {
        return -1;
    }

    gpio_low(CAR_ICM45686_CS_PIN);
    (void)icm45686_spi_exchange((uint8)(reg & 0x7FU));
    for (index = 0U; index < len; index++)
    {
        (void)icm45686_spi_exchange(data[index]);
    }
    gpio_high(CAR_ICM45686_CS_PIN);

    return 0;
}

void icm45686_port_delay_us(uint32_t us)
{
    system_delay_us(us);
}

uint32 icm45686_port_get_ms(void)
{
    return system_get_ms();
}

uint8 icm45686_port_int1_level(void)
{
    return gpio_get_level(CAR_ICM45686_INT1_PIN);
}

#ifndef SPI_DRIVER_H
#define SPI_DRIVER_H

#include <stm32f446xx.h>
#include <stdbool.h>

#define SPI_TIMEOUT_MAX  100000U

void init_spi();
void systick_delay_init(void);
void ili9341_reset();
void cs_low(void);
void cs_high(void);
void dc_command(void);
void dc_data(void);
void delay_ms(uint32_t ms);
void delay_us(uint32_t us);
uint8_t spi_tx_raw(const uint8_t* data, uint16_t size);
uint8_t spi_rx_raw(uint8_t* data, uint16_t size);
uint8_t spi_tx_dma(const uint8_t *data, uint16_t size);

#endif
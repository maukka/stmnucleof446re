#ifndef DMA_DRIVER_H
#define DMA_DRIVER_H
#include <stdint.h> 
#define DMA_TIMEOUT_MAX  100000U

void init_dma_driver();
uint8_t dma_spi_tx_start(const uint8_t *data, uint16_t size);
uint8_t dma_spi_tx_wait_complete(void);

#endif
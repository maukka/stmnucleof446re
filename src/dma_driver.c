#include <stm32f446xx.h>
#include <stddef.h>
#include "../include/dma_driver.h"

void init_dma_driver(){

	// Enable clock for DMA2
	RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;

	// Configure DMA stream registers

	//Disable DMA
	DMA2_Stream3->CR &= ~DMA_SxCR_EN;

	//Clear channel bits
	DMA2_Stream3->CR &= ~(0x7U << 25);
	// Select channel 3, the SPI1 is connected here
	DMA2_Stream3->CR |= (0x3U << 25);

	//MSIZE 8 bit (1byte)
	DMA2_Stream3->CR &= ~(0x3 << 13);

	//PSIZE 8 bit (1byte)
	DMA2_Stream3->CR &= ~(0x3 << 11);

	//MINC memory increment enabled
	DMA2_Stream3->CR |= (0x1 << 10);

	// PINC disabled
	DMA2_Stream3->CR &= ~(1U << 9);
	// CIRC disabled
	DMA2_Stream3->CR &= ~(1U << 8);   

	//DIR Memory to pheriperal
	DMA2_Stream3->CR &= ~(0x3 << 6);
	DMA2_Stream3->CR |= (0x1 << 6);

	//Pheriperal data address
	DMA2_Stream3->PAR = (uint32_t)&(SPI1->DR);
}

uint8_t dma_spi_tx_start(const uint8_t *data, uint16_t size){

	if (data == NULL && size > 0){
		return -1;
	}
	//Set size of data items to transfer
	DMA2_Stream3->NDTR = size;
	DMA2_Stream3->M0AR = (uint32_t)data;
	DMA2_Stream3->FCR &= ~(0x7U << 3);
	DMA2_Stream3->FCR |= (0x5 << 3);
	DMA2_Stream3->CR |= DMA_SxCR_EN; 

	return 0;
}

uint8_t dma_spi_tx_wait_complete(void){

	uint32_t timeout = DMA_TIMEOUT_MAX;

	// Poll stream 3 transfer complete flag 
	while( !(DMA2->LISR & DMA_LISR_TCIF3) ){
		if(--timeout == 0){
			return -1;
		}
	}

	// Clear stream 3 transfer complete flag for next round
	DMA2->LIFCR = DMA_LIFCR_CTCIF3;

	timeout = DMA_TIMEOUT_MAX;
	// Poll last byte ready from SPI1
	while (SPI1->SR & SPI_SR_BSY){
		if(--timeout == 0){
			return -1;
		}
	}

	return 0;
}
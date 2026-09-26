#include <stddef.h>
#include "../include/dma_driver.h"
#include "../include/spi_driver.h"

/**
 * Initializes clocks and registers for GBIOA and GBIOB pins
 * We need these for display data lines (TX, RX).
 * 
 */
static void init_gbio_clocks(void) {
    // 1. Kytketään kellot GPIOA ja GPIOB väylille
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN;

    // 2. Nollataan PA5, PA6, PA7 MODER-bitit (3U = 0b11)
    GPIOA->MODER &= ~((3U << (5 * 2)) | (3U << (6 * 2)) | (3U << (7 * 2)));

    // 3. Asetetaan PA5, PA6, PA7 -> Alternate Function (2U = 0b10)
    GPIOA->MODER |= (2U << (5 * 2)) | (2U << (6 * 2)) | (2U << (7 * 2));

    // 4. PA8 (LED) -> Output Mode (1U = 0b01)
    GPIOA->MODER &= ~(3U << (8 * 2));
    GPIOA->MODER |=  (1U << (8 * 2));

    // 5. Tyhjennetään AFRL-bitit PA5, PA6, PA7
    GPIOA->AFR[0] &= ~((0xFU << (5 * 4)) | (0xFU << (6 * 4)) | (0xFU << (7 * 4)));

    // 6. Asetetaan AF5 (SPI1) pinneille PA5, PA6, PA7
    GPIOA->AFR[0] |= (5U << (5 * 4)) | (5U << (6 * 4)) | (5U << (7 * 4));

    // 7. Nopea lähtö (High Speed 0b11) PA5, PA6, PA7
    GPIOA->OSPEEDR &= ~((3U << (5 * 2)) | (3U << (6 * 2)) | (3U << (7 * 2)));
    GPIOA->OSPEEDR |=  ((3U << (5 * 2)) | (3U << (6 * 2)) | (3U << (7 * 2)));

    // 8. No pull-up / pull-down PA5, PA6, PA7
    GPIOA->PUPDR &= ~((3U << (5 * 2)) | (3U << (6 * 2)) | (3U << (7 * 2)));

    // 9. GPIOB PB0 (DC), PB1 (RESET), PB6 (CS) -> Output Mode (0b01)
    GPIOB->MODER &= ~((3U << (0 * 2)) | (3U << (1 * 2)) | (3U << (6 * 2)));
    GPIOB->MODER |=  ((1U << (0 * 2)) | (1U << (1 * 2)) | (1U << (6 * 2)));

    // Push-pull lähtö
    GPIOB->OTYPER &= ~((1U << 0) | (1U << 1) | (1U << 6));

    // Oletuksena pinnat HIGH-tilaan (CS, DC, RESET)
    GPIOB->BSRR = (1U << 0) | (1U << 1) | (1U << 6);
}

/**
 * Init SPI1 registers. Defines the bit rate we use, data format, 
 * byte order mode, slave management, interrupts and enabling the SPI peripheral.
 */
static void init_spi_registers(){

	// Enable SPI1 clock in APB bus
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    (void)RCC->APB2ENR; // Dummy read -> just for testing if this helps

    // 2. Shut down SPI
    SPI1->CR1 = 0;

    // Configuration of SPI1:
    // - Master Mode (MSTR = bit 2)
    // - Software Slave Management (SSM = bit 9, SSI = bit 8) -> mandatory we contro chip select manually!
    // - Clock frequence divider (BR[2:0]): Slow clock at fisrt since jumper cables 
	//  are really sensible of errors(divider example. /32 or /64)
    //   BR = 100b (bitit 5:3) -> APB2 / 32
    // - CPOL = 0, CPHA = 0 (Mode 0 for ILI9341)
    //SPI1->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI | (0x4U << SPI_CR1_BR_Pos);
	// This is slow speed for testing with jump wires
	//SPI1->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI | (0x7U << SPI_CR1_BR_Pos);
	SPI1->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI | (0x0U << SPI_CR1_BR_Pos);

    // Enable 8-bit DFF / SSOE
    SPI1->CR2 = 0; // Standardi 8-bittinen tila STM32F4:ssä

	// Enable DMA transfer
    SPI1->CR2 |= SPI_CR2_TXDMAEN;	
    // Enable SPI1 (SPE = bit 6)
    SPI1->CR1 |= SPI_CR1_SPE;
}

/**
 * Initializes SysTick timer for microsecond-accurate delays.
 * Uses HCLK directly as clock source (CLKSOURCE = 1), no interrupt (polling mode).
 * Call this once during startup, after SystemCoreClock is correctly set
 * (i.e. after SystemInit() / clock configuration).
 */
void systick_delay_init(void){

	// Use core clock (HCLK) directly as SysTick clock source
	SysTick->CTRL |= SysTick_CTRL_CLKSOURCE_Msk;

	// Disable SysTick and its interrupt for now; we will drive it manually per delay call
	SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
	SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;
}

/**
 * Blocking delay in microseconds.
 * Reconfigures LOAD for exactly 1 microsecond per iteration and polls COUNTFLAG.
 */
void delay_us(uint32_t us){

	// Ticks needed for 1 microsecond at current SystemCoreClock (Hz)
	uint32_t ticks_per_us = SystemCoreClock / 1000000U;

	// SysTick reload register is 24-bit; ticks_per_us must fit in 24 bits
	SysTick->LOAD = (ticks_per_us - 1U) & 0x00FFFFFFU;

	// Clear current value register (also clears COUNTFLAG)
	SysTick->VAL = 0U;

	// Start the counter
	SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;

	for (uint32_t i = 0; i < us; i++){
		// Wait until COUNTFLAG is set (counter reached 0)
		while (!(SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk)){
			// busy wait
		}
		// Reading/checking CTRL above already clears COUNTFLAG automatically
	}

	// Stop the counter after we're done
	SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
}

/**
 * Blocking delay in milliseconds. Built on top of delay_us.
 */
void delay_ms(uint32_t ms){

	while (ms--){
		delay_us(1000U);
	}
}

/**
 * RESET pin PB1 needs to be pulsed before we can send any data to bus.
 * This way we make sure the device is not unstable state after initialization.
 */
void ili9341_reset(){

	// RESET HIGH (PB1)
    GPIOB->BSRR = (1U << 1);
    delay_ms(10);
    
    // RESET LOW
    GPIOB->BSRR = (1U << (1 + 16));
    delay_ms(50);
    
    // RESET HIGH 
    GPIOB->BSRR = (1U << 1);
    delay_ms(120);
}

/**
 * Set chip select to low. Reserves the bus line to this pheriperal
 */
void cs_low(void){

	GPIOB->BSRR = (1U << (6 + 16));
}

/**
 * Set chip select to high. Releases the bus line for other devices to communicate.
 */
void cs_high(void){

	GPIOB->BSRR = (1U << 6);
}

/**
 * Set the DC register to inform pheriperal we send commands instead of data
 */
void dc_command(void){

	GPIOB->BSRR = (1U << (0 + 16));
}

/**
 * Set the DC register to inform pheriperal we send data
 */
void dc_data(void){

	GPIOB->BSRR = (1U << 0);
}

/**
 * Calls the static functions to initialize GPIOP and SPI.
 * This way we can split the functionality to more managble size and it is easier
 * to maintain and change, One function handles the GPIO part and one SPI.
 */
void init_spi(){

	init_gbio_clocks();
	init_spi_registers();
}

uint8_t spi_rx_raw(uint8_t* data, uint16_t size){

	if(data == NULL && size > 0){
		return -1;
	}

	uint32_t timeout = SPI_TIMEOUT_MAX;

	// Wait RXNE bit is set to one before reading data
	for(uint16_t i = 0; i < size; i++){

		// We need to write dummy data to bus, so we generate needed clock pulses for each reading
		while (!(SPI1->SR & SPI_SR_TXE)){
			if(--timeout == 0){
				return -1;
			}
		}
		// No need for type cast...just dummy 0 data
		SPI1->DR = 0x00;
		timeout = SPI_TIMEOUT_MAX;
		while (!(SPI1->SR & SPI_SR_RXNE)){
			if(--timeout == 0){
				return -1;
			}
		}

		timeout = SPI_TIMEOUT_MAX;
		data[i] = (uint8_t)SPI1->DR;
	}

	timeout = SPI_TIMEOUT_MAX;
	while (SPI1->SR & SPI_SR_BSY){
		if(--timeout == 0){
			return -1;
		}
	}

	return 0;
}

uint8_t spi_tx_raw(const uint8_t* data, uint16_t size){

	if(data == NULL && size > 0){
		return -1;
	}

	uint32_t timeout = SPI_TIMEOUT_MAX;

	for(uint16_t i = 0; i < size; i++){

		while (!(SPI1->SR & SPI_SR_TXE)){
			if(--timeout == 0){
				return -1;
			}
		}

		timeout = SPI_TIMEOUT_MAX;
		// Need explicit typecasting to uint8_t compiler might change this to uint16_t 
		// which brakes the data structure
		*(volatile uint8_t *)&SPI1->DR = data[i];
	}

	timeout = SPI_TIMEOUT_MAX;
	//Wait that data is completly send from buffer before setting CS high
	while(SPI1->SR & SPI_SR_BSY){
		if(--timeout == 0){
			return -1;
		}
	}
	return 0;
}

uint8_t spi_tx_dma(const uint8_t *data, uint16_t size){

	uint8_t status = dma_spi_tx_start(data, size);
	if (status != 0){
		return status;
	}

	return dma_spi_tx_wait_complete();
}
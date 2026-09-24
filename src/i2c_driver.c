#include "../include/i2c_driver.h"

#define I2C_TIMEOUT_MAX  100000U

static void i2c_clock_enable(I2C_TypeDef *i2c)
{
    if (i2c == I2C1)
		RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;
    else if (i2c == I2C2) RCC->APB1ENR |= RCC_APB1ENR_I2C2EN;
    else if (i2c == I2C3) RCC->APB1ENR |= RCC_APB1ENR_I2C3EN;
}

static void gpio_clock_enable(GPIO_TypeDef *port)
{
    if (port == GPIOA)      
		RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    else if (port == GPIOB) RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    else if (port == GPIOC) RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    else if (port == GPIOF) RCC->AHB1ENR |= RCC_AHB1ENR_GPIOFEN;
    else if (port == GPIOH) RCC->AHB1ENR |= RCC_AHB1ENR_GPIOHEN;
    // laajenna tarvittaessa muille porteille
}

static void i2c_clear_error_flags(I2C_TypeDef *i2c)
{
    i2c->SR1 &= ~(I2C_SR1_AF | I2C_SR1_BERR | I2C_SR1_ARLO | I2C_SR1_OVR);
}

static I2C_status_t i2c_check_errors(I2C_TypeDef *i2c)
{
    uint32_t sr1 = i2c->SR1;

    if (sr1 & I2C_SR1_AF)
    {
        i2c->SR1 &= ~I2C_SR1_AF;
        return I2C_ERR_NACK;
    }
    if (sr1 & I2C_SR1_BERR)
    {
        i2c->SR1 &= ~I2C_SR1_BERR;
        return I2C_ERR_BUS;
    }
    if (sr1 & I2C_SR1_ARLO)
    {
        i2c->SR1 &= ~I2C_SR1_ARLO;
        return I2C_ERR_ARBITRATION;
    }

    return I2C_OK;
}

void init_i2c_driver(I2C_TypeDef *i2c, GPIO_TypeDef *scl_port, uint8_t scl_pin, GPIO_TypeDef *sda_port, uint8_t sda_pin){

    gpio_clock_enable(scl_port);
    gpio_clock_enable(sda_port);
    i2c_clock_enable(i2c);

    // MODER: alternate function -mode (b10) kummallekin pinnille erikseen
    scl_port->MODER &= ~(3U << (scl_pin * 2));
    scl_port->MODER |=  (2U << (scl_pin * 2));
    sda_port->MODER &= ~(3U << (sda_pin * 2));
    sda_port->MODER |=  (2U << (sda_pin * 2));

    // OTYPER: open-drain
    scl_port->OTYPER |= (1U << scl_pin);
    sda_port->OTYPER |= (1U << sda_pin);

    // PUPDR: pull-up
    scl_port->PUPDR &= ~(3U << (scl_pin * 2));
    scl_port->PUPDR |=  (1U << (scl_pin * 2));
    sda_port->PUPDR &= ~(3U << (sda_pin * 2));
    sda_port->PUPDR |=  (1U << (sda_pin * 2));

    // AFR (index and shift are calculated from pin number)
    uint8_t scl_afr_idx = scl_pin / 8;
    uint8_t sda_afr_idx = sda_pin / 8;
    uint8_t scl_shift = (scl_pin % 8) * 4;
    uint8_t sda_shift = (sda_pin % 8) * 4;

    scl_port->AFR[scl_afr_idx] &= ~(0xFU << scl_shift);
    scl_port->AFR[scl_afr_idx] |=  (4U  << scl_shift);
    sda_port->AFR[sda_afr_idx] &= ~(0xFU << sda_shift);
    sda_port->AFR[sda_afr_idx] |=  (4U  << sda_shift);

    // I2C-registers (Common not depend of ports)
    i2c->CR1 |= I2C_CR1_SWRST;
    i2c->CR1 &= ~I2C_CR1_SWRST;
    i2c->TRISE = 17;
    i2c->CR2 = 16;
    i2c->CCR |= 80;
    i2c->CR1 |= I2C_CR1_PE;
}

I2C_status_t send_start_sequence(I2C_TypeDef *i2c){

	uint32_t timeout = I2C_TIMEOUT_MAX;

    // Generate START condition
    i2c->CR1 |= I2C_CR1_START;

    // Wait for SB flag (Start Bit sent) with timeout
    while (!(i2c->SR1 & I2C_SR1_SB))
    {
        if (--timeout == 0)
            return I2C_ERR_TIMEOUT;
    }

    return I2C_OK;

}

I2C_status_t send_stop_sequence(I2C_TypeDef *i2c){

	uint32_t timeout = I2C_TIMEOUT_MAX;

    // Generate stop condition
    i2c->CR1 |= I2C_CR1_STOP;

    // Wait for SB flag (Start Bit sent) with timeout
    while (!(i2c->SR2 & I2C_SR2_BUSY))
    {
        if (--timeout == 0)
            return I2C_ERR_TIMEOUT;
    }

    return I2C_OK;

}

I2C_status_t read_data(I2C_TypeDef *i2c, uint8_t slave_addr, uint8_t *data, uint16_t len)
{
    uint32_t timeout;
    I2C_status_t status;

    if (len == 0)
        return I2C_OK;

    status = send_start_sequence(i2c);
    if (status != I2C_OK){
        return status;
	}

	i2c_clear_error_flags(i2c);

    // Enable ACK by default (needed before address phase for multi-byte reads)
    i2c->CR1 |= I2C_CR1_ACK;

    // Send 7-bit slave address, read direction (LSB = 1)
    i2c->DR = (uint8_t)((slave_addr << 1) | 0x01U);

    // Wait for ADDR or AF (NACK), same as write_data
    timeout = I2C_TIMEOUT_MAX;
    while (!(i2c->SR1 & I2C_SR1_ADDR))
    {
  		status = i2c_check_errors(i2c);
		if (status != I2C_OK)
		{
			send_stop_sequence(i2c);
			return status;
		}
        if (--timeout == 0)
        {
            send_stop_sequence(i2c);
            return I2C_ERR_TIMEOUT;
        }
    }

    if (len == 1)
    {
        // Single-byte read: must clear ACK and queue STOP
        // BEFORE clearing ADDR, per RM0390 (critical section)
        i2c->CR1 &= ~I2C_CR1_ACK;

        (void)i2c->SR1;
        (void)i2c->SR2;   // clears ADDR

        i2c->CR1 |= I2C_CR1_STOP;

        timeout = I2C_TIMEOUT_MAX;
        while (!(i2c->SR1 & I2C_SR1_RXNE))
        {
			status = i2c_check_errors(i2c);
			if (status != I2C_OK)
			{
				send_stop_sequence(i2c);
				return status;
			}
            if (--timeout == 0)
                return I2C_ERR_TIMEOUT;
        }
        data[0] = (uint8_t)i2c->DR;

        return I2C_OK;
    }

    // Multi-byte read: clear ADDR normally, ACK stays enabled for now
    (void)i2c->SR1;
    (void)i2c->SR2;

    for (uint16_t i = 0; i < len; i++)
    {
        if (i == (uint16_t)(len - 2))
        {
            // Second-to-last byte: NACK the *next* (last) byte,
            // and queue STOP so it takes effect right after it
            i2c->CR1 &= ~I2C_CR1_ACK;
            i2c->CR1 |= I2C_CR1_STOP;
        }

        timeout = I2C_TIMEOUT_MAX;
        while (!(i2c->SR1 & I2C_SR1_RXNE))
        {
			status = i2c_check_errors(i2c);
			if (status != I2C_OK)
			{
				send_stop_sequence(i2c);
				return status;
			}
            if (--timeout == 0)
                return I2C_ERR_TIMEOUT;
        }
        data[i] = (uint8_t)i2c->DR;
    }

    return I2C_OK;
}

// internal, no STOP — shared by write_data and write_then_read
static I2C_status_t write_bytes_no_stop(I2C_TypeDef *i2c, uint8_t slave_addr, uint8_t *data, uint16_t len)
{
    uint32_t timeout;
    I2C_status_t status;

	// Steps to write data to slave
	// 1. Send start address to bus and wait ack
	// 2. After succesfully send of slave address and gettig ack back
	//    we start to send data byte by byte to slave
	// 3. Finally if we succesfully managed to send all bytes
	//    we wait that final byte is completly send before sending the
	//    stop sequence.

    status = send_start_sequence(i2c);

    if (status != I2C_OK){
        return status;
	}

	i2c_clear_error_flags(i2c);

    // Send 7-bit slave address, write direction (LSB = 0)
    i2c->DR = (uint8_t)(slave_addr << 1);

    // Wait for ADDR (address matched/acknowledged) or AF (NACK) with timeout
    timeout = I2C_TIMEOUT_MAX;
    while (!(i2c->SR1 & I2C_SR1_ADDR))
    {
		// Check if slave did not send ACK for address.
		// If not return error status. This way we break the loop without waiting
		// the timeout to trigger.
		status = i2c_check_errors(i2c);
		if (status != I2C_OK)
		{
			send_stop_sequence(i2c);
			return status;
		}
		// No NACK, but we must check the timeout.
		// If it has reached it limits send timeout error.
        if (--timeout == 0)
        {
            send_stop_sequence(i2c);
            return I2C_ERR_TIMEOUT;
        }
    }

    // Clear ADDR: must read SR1 then SR2, in this order
    (void)i2c->SR1;
    (void)i2c->SR2;

    for (uint16_t i = 0; i < len; i++)
    {
        timeout = I2C_TIMEOUT_MAX;
		// Wait that data has been completly transferred from DR register
		// before setting new data to it.
        while (!(i2c->SR1 & I2C_SR1_TXE))
        {
			status = i2c_check_errors(i2c);
			if (status != I2C_OK)
			{
				send_stop_sequence(i2c);
				return status;
			}
            if (--timeout == 0)
            {
                send_stop_sequence(i2c);
                return I2C_ERR_TIMEOUT;
            }
        }
		// Set new data to data register
        i2c->DR = data[i];
    }

    // Wait for BTF (last byte fully shifted out) before issuing STOP
    timeout = I2C_TIMEOUT_MAX;
    while (!(i2c->SR1 & I2C_SR1_BTF))
    {
        if (--timeout == 0)
        {
            send_stop_sequence(i2c);
            return I2C_ERR_TIMEOUT;
        }
    }

	return I2C_OK;

}

I2C_status_t write_data(I2C_TypeDef *i2c, uint8_t slave_addr, uint8_t *data, uint16_t len)
{
   I2C_status_t status = write_bytes_no_stop(i2c, slave_addr, data, len);
    if (status != I2C_OK)
        return status;
    return send_stop_sequence(i2c);
}

I2C_status_t write_then_read(I2C_TypeDef *i2c, uint8_t slave_addr,
                              uint8_t *write_buf, uint16_t write_len,
                              uint8_t *read_buf, uint16_t read_len)
{
    I2C_status_t status = write_bytes_no_stop(i2c, slave_addr, write_buf, write_len);
    if (status != I2C_OK)
        return status;

    // repeated-START: call send_start_sequence again WITHOUT a STOP before it
    return read_data(i2c, slave_addr, read_buf, read_len);
}

#ifndef I2C_DRIVER_H
#define I2C_DRIVER_H

#include <stm32f446xx.h>

typedef enum {
    I2C_OK,
    I2C_ERR_NACK,
    I2C_ERR_TIMEOUT,
    I2C_ERR_BUS_BUSY,
    I2C_ERR_BUS,          // new: BERR — misplaced START/STOP mid-byte
    I2C_ERR_ARBITRATION,  // new: ARLO — lost arbitration (multi-master)
} I2C_status_t;

void init_i2c_driver(I2C_TypeDef *i2c, GPIO_TypeDef *scl_port, uint8_t scl_pin, GPIO_TypeDef *sda_port, uint8_t sda_pin);
I2C_status_t send_start_sequence(I2C_TypeDef *i2c);
I2C_status_t write_data(I2C_TypeDef *i2c, uint8_t slave_addr, uint8_t *data, uint16_t len);
I2C_status_t send_stop_sequence(I2C_TypeDef *i2c);
I2C_status_t read_data(I2C_TypeDef *i2c, uint8_t slave_addr, uint8_t *data, uint16_t len);
I2C_status_t write_then_read(I2C_TypeDef *i2c, uint8_t slave_addr,
                              uint8_t *write_buf, uint16_t write_len,
                              uint8_t *read_buf, uint16_t read_len);

#endif
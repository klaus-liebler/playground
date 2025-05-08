#pragma once
 
#include <stdio.h>
#include <driver/i2c.h>
#include <driver/gpio.h>

class I2C{
    private:
    SemaphoreHandle_t lock;
    const i2c_port_t port;
    public:
    I2C(const i2c_port_t port);
    esp_err_t Init(gpio_num_t sda, gpio_num_t scl);
    esp_err_t ReadReg(uint8_t address7bit, uint8_t reg_addr, uint8_t *reg_data, size_t len);
    esp_err_t Read(uint8_t address7bit, uint8_t *data, size_t len);
    esp_err_t WriteReg(uint8_t address7bit, uint8_t reg_addr, uint8_t *reg_data, size_t len);
    esp_err_t WriteSingleReg(uint8_t address7bit, uint8_t reg_addr, uint8_t reg_data);
    esp_err_t Write(uint8_t address7bit, uint8_t *data, size_t len);
    esp_err_t IsAvailable(uint8_t adress7bit);
    esp_err_t SearchBus();
};
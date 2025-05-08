
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include "include/i2c.hh"

static const char *TAG = "I2C_DEV";

#define I2C_MASTER_FREQ_HZ 100000 /*!< I2C master clock frequency */

I2C::I2C(const i2c_port_t port):port(port){
    this->lock = xSemaphoreCreateMutex();
    if( this->lock == NULL )
    {
        ESP_LOGE(TAG, "this->lock == NULL");
    }
}

esp_err_t I2C::Init(gpio_num_t sda, gpio_num_t scl)
{
    esp_err_t ret;
	i2c_config_t conf;
	conf.mode = I2C_MODE_MASTER;
	conf.sda_io_num = sda;
	conf.sda_pullup_en = GPIO_PULLUP_DISABLE;
	conf.scl_io_num = scl;
	conf.scl_pullup_en = GPIO_PULLUP_DISABLE;
	conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
	conf.clk_flags = 0;
	ret = i2c_param_config(this->port, &conf);
	if (ret == ESP_OK)
	{
		return i2c_driver_install(this->port, conf.mode, 0, 0, 0);
	}
    
	return ret;
}

esp_err_t I2C::WriteSingleReg(uint8_t address7bit, uint8_t reg_addr, uint8_t reg_data){
    return WriteReg(address7bit, reg_addr, &reg_data, 1);
}

esp_err_t I2C::SearchBus(){
    ESP_LOGI(TAG, "Search Bus");
    int cnt=0;
    for(int i=0;i<128;i++){
        if(IsAvailable(i)==ESP_OK){
            ESP_LOGI(TAG, "Device found at address 0x%02X", i);
            cnt++;
        }
    }
    ESP_LOGI(TAG, "Search finished %d device(s) found.", cnt);
    return ESP_OK;
}

esp_err_t I2C::WriteReg(uint8_t address7bit, uint8_t reg_addr, uint8_t *reg_data, size_t len)
{
    if (!xSemaphoreTake(lock, pdMS_TO_TICKS(1000)))
    {
        ESP_LOGE(TAG, "Could not take port mutex %d", port);
        return ESP_ERR_TIMEOUT;
    }
    esp_err_t espRc;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (address7bit << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg_addr, true);
    i2c_master_write(cmd, (uint8_t *)reg_data, len, true);
    i2c_master_stop(cmd);

    espRc = i2c_master_cmd_begin(port, cmd, 10 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    xSemaphoreGive(lock);
    if(espRc!=ESP_OK){
        ESP_LOGE(TAG, "Error %d", espRc);
    }
    return espRc;
}

esp_err_t I2C::ReadReg(uint8_t address7bit, uint8_t reg_addr, uint8_t *reg_data, size_t len)
{
    if (!xSemaphoreTake(lock, pdMS_TO_TICKS(1000)))
    {
        ESP_LOGE(TAG, "Could not take port mutex %d", port);
        return ESP_ERR_TIMEOUT;
    }
    esp_err_t espRc;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (address7bit << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg_addr, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (address7bit << 1) | I2C_MASTER_READ, true);
    if (len > 1)
    {
        i2c_master_read(cmd, reg_data, len - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, reg_data + len - 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    espRc = i2c_master_cmd_begin(port, cmd, 10 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    xSemaphoreGive(lock);
    return espRc;
}

esp_err_t I2C::IsAvailable(uint8_t address7bit){

    if (!xSemaphoreTake(lock, pdMS_TO_TICKS(1000)))
    {
        ESP_LOGE(TAG, "Could not take port mutex %d", port);
        return ESP_ERR_TIMEOUT;
    }
    //Nothing to init. Just check if it is there...
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (address7bit << 1) | I2C_MASTER_WRITE  , true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(port, cmd, pdMS_TO_TICKS(50));
    i2c_cmd_link_delete(cmd);
    xSemaphoreGive(lock);
    return ret; 
}

esp_err_t I2C::Read(uint8_t address7bit, uint8_t *data, size_t len){
    if (!xSemaphoreTake(lock, pdMS_TO_TICKS(1000)))
    {
        ESP_LOGE(TAG, "Could not take port mutex %d", port);
        return ESP_ERR_TIMEOUT;
    }
    esp_err_t espRc;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (address7bit << 1) | I2C_MASTER_READ, true);
    if (len > 1)
    {
        i2c_master_read(cmd, data, len - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, data + len - 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    espRc = i2c_master_cmd_begin(port, cmd, 10 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
     xSemaphoreGive(lock);
    return espRc;
}

esp_err_t I2C::Write(uint8_t address7bit, uint8_t *data, size_t len){
    if (!xSemaphoreTake(lock, pdMS_TO_TICKS(1000)))
    {
        ESP_LOGE(TAG, "Could not take port mutex %d", port);
        return ESP_ERR_TIMEOUT;
    }
    esp_err_t espRc;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, address7bit << 1 | I2C_MASTER_WRITE, true);
    i2c_master_write(cmd, data, len, true);
    i2c_master_stop(cmd);
    espRc = i2c_master_cmd_begin(port, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);
    xSemaphoreGive(lock);
    return espRc;
}
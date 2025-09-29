#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"

#include <vl53l0x.hh>
#include <mpu6050.hh>
#include <esp_log.h>
#include <common-esp32.hh>
#define TAG "MAIN"

extern "C" void app_main(void)
{
    I2C::Init(I2C_NUM_0, GPIO_NUM_22, GPIO_NUM_21, 0);
    iI2CPort *i2c_port_interface = I2C::GetPort_DoNotForgetToDelete(I2C_NUM_0);
    i2c_port_interface->Discover();
    

    VL53L0X::M* vl53l0xdev= new VL53L0X::M(i2c_port_interface);
    MPU6050::M* mpu6050dev = new MPU6050::M(i2c_port_interface);

    mpu6050dev->Config(MPU6050::ACCELERATION_FS::_4G, MPU6050::GYRO_FS::_1000DPS);
    mpu6050dev->WakeUp();

    MPU6050::xyz_f32 gyro_value;
   
    while (true)
    {
        //vl53l0xdev->Loop(millis());
        //if(vl53l0xdev->HasValidData()){
        //    ESP_LOGI(TAG, "%d", vl53l0xdev->ReadOut());
        //}
        mpu6050dev->GetGyro(&gyro_value);
        ESP_LOGI(TAG, "%4.1f %4.1f %4.1f", gyro_value.x, gyro_value.y, gyro_value.z);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
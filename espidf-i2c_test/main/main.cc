#include <stdio.h>
#include <common-esp32.hh>
#include <pca9555.hh>
#include <pca9685.hh>
#include <i2c.hh>
#include <esp_log.h>
#define TAG "MAIN"

PCA9555::M *pca9555{nullptr};
PCA9685::M *pca9685{nullptr};



extern "C" void app_main(void)
{
    ESP_ERROR_CHECK(I2C::Init(I2C_NUM_0, GPIO_NUM_38, GPIO_NUM_37));
    iI2CPort* i2cPort= I2C::GetPort_DoNotForgetToDelete(I2C_NUM_0);
    pca9555 = new PCA9555::M(i2cPort, PCA9555::Device::Dev0, 0, 0xFFFF, 0xFFFF);
    ERRORCODE_CHECK(pca9555->Setup());
    ESP_ERROR_CHECK(I2C::IsAvailable(I2C_NUM_0, 0x20));
    // while(true){
    //     uint8_t tmp[2];
    //     ESP_ERROR_CHECK(I2C::ReadReg(I2C_NUM_0, 0x20, 0, tmp, 2));
    //     ESP_LOGI(TAG, "Inputs %02X%02X", tmp[1], tmp[0]);
    //     //ERRORCODE_CHECK(pca9555->Update());
    //     //uint16_t inputs =  pca9555->GetCachedInput();
    //     //ESP_LOGI(TAG, "Inputs %04X", inputs);
    //     esp32::delayMs(1000);
    // }
    pca9685 = new PCA9685::M(i2cPort, PCA9685::Device::Dev00, PCA9685::InvOutputs::NotInvOutputs, PCA9685::OutputDriver::TotemPole, PCA9685::OutputNotEn::OutputNotEn_0, PCA9685::Frequency::Frequency_400Hz);
    ERRORCODE_CHECK(pca9685->Setup());

    while(true){
        pca9685->SetOutput(0, 0xFFFF);
        pca9685->Loop();
        vTaskDelay(pdMS_TO_TICKS(1000));
        pca9685->SetOutput(0, 0);
        pca9685->Loop();
        vTaskDelay(pdMS_TO_TICKS(1000));


    }
    while(true){
        for(int i=4;i<14;i++){
            pca9685->SetOutput(i, i*4096);
        }
        pca9685->Loop();
        vTaskDelay(pdMS_TO_TICKS(1000));
        for(int i=4;i<14;i++){
            pca9685->SetOutput(i, 0);
        }
        pca9685->Loop();
        vTaskDelay(pdMS_TO_TICKS(1000));

        for(int i=2;i<5;i++){
            pca9685->SetOutput(i, (16-i)*4096);
        }
        pca9685->Loop();
        vTaskDelay(pdMS_TO_TICKS(1000));
        for(int i=2;i<5;i++){
            pca9685->SetOutput(i, 0);
        }
        pca9685->Loop();
        vTaskDelay(pdMS_TO_TICKS(1000));
        
    }
    
}
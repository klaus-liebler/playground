#include <cstdio>
#include <cstring>
#include <cctype>
#include "stm32f4xx_hal.h"
#include "firfilter.hh"
#include "mpu6050.hh"
#include "usbd_cdc_if.h"

extern I2C_HandleTypeDef hi2c1;
extern UART_HandleTypeDef huart1;

#include "fdacoefs.h"


FIR::Filter<BL>* filter;
MPU6050::MPU6050 *mpu6050;


extern "C" void app_main(void) {

	//query i2c bus

	printf("Device Search\r\n");
	int n=0;
	for(int i=0;i<256;i+=2){
		if(HAL_I2C_IsDeviceReady(&hi2c1, i, 1, 200)!=HAL_OK) continue;
		printf("Device found at 8bit address %d\r\n", i);
		n++;
	}
	printf("Found %d devices\r\n", n);


	filter = new FIR::Filter<BL>(B);
	mpu6050 = new MPU6050::MPU6050(&hi2c1);
	assert(mpu6050->Setup());

	char txBuf[32];

	while (1) {
		mpu6050->UpdateAccel();
		float Ax=mpu6050->GetAx();
		float Ax_filtered=filter->Update(Ax);
		//MPU6050_Read_Gyro();
		//printf("Ax=%.2f, Ay=%.2f, Az=%.2f, Gx=%.2f, Gy=%.2f, Gz=%.2f \r\n", Ax, Ay, Az, Gx, Gy, Gz);
		printf("%.4f, %.4f\r\n", Ax, Ax_filtered);
		sprintf(txBuf, "%.4f, %.4f\r\n", Ax, Ax_filtered);
		CDC_Transmit_FS((uint8_t*) txBuf, strlen(txBuf));
		HAL_Delay(10);

	}

}


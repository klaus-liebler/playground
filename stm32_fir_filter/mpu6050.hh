#pragma once

#include "stm32f4xx_hal.h"

namespace MPU6050 {

constexpr uint8_t MPU6050_ADDR { 0xD0 };

constexpr uint8_t SMPLRT_DIV_REG { 0x19 };
constexpr uint8_t GYRO_CONFIG_REG { 0x1B };
constexpr uint8_t ACCEL_CONFIG_REG { 0x1C };
constexpr uint8_t ACCEL_XOUT_H_REG { 0x3B };
constexpr uint8_t TEMP_OUT_H_REG { 0x41 };
constexpr uint8_t GYRO_XOUT_H_REG { 0x43 };
constexpr uint8_t PWR_MGMT_1_REG { 0x6B };
constexpr uint8_t WHO_AM_I_REG { 0x75 };

class MPU6050 {
private:
	float A[3] { 0.0f };
	float G[3] { 0.0f };
	I2C_HandleTypeDef* i2c{nullptr};
public:
	MPU6050(I2C_HandleTypeDef* i2c):i2c(i2c){}
	bool Setup() {
		uint8_t check { 0 };
		uint8_t Data { 0 };
		// check device ID WHO_AM_I

		if (HAL_I2C_Mem_Read(i2c, MPU6050_ADDR, WHO_AM_I_REG, 1, &check, 1,
				1000) != HAL_OK) {
			printf("Device not found\r\n");
			return false;
		}

		if (check != 0x68) {
			printf("Invalid Device ID\r\n");
			return false; // 0x68 will be returned by the sensor if everything goes well
		}

		// power management register 0X6B we should write all 0's to wake the sensor up
		Data = 0;
		HAL_I2C_Mem_Write(i2c, MPU6050_ADDR, PWR_MGMT_1_REG, 1, &Data, 1,
				1000);

		// Set DATA RATE of 1KHz by writing SMPLRT_DIV register
		Data = 0x07;
		HAL_I2C_Mem_Write(i2c, MPU6050_ADDR, SMPLRT_DIV_REG, 1, &Data, 1,
				1000);

		// Set accelerometer configuration in ACCEL_CONFIG Register
		// XA_ST=0,YA_ST=0,ZA_ST=0, FS_SEL=0 -> ± 2g
		Data = 0x00;
		HAL_I2C_Mem_Write(i2c, MPU6050_ADDR, ACCEL_CONFIG_REG, 1, &Data, 1,
				1000);

		// Set Gyroscopic configuration in GYRO_CONFIG Register
		// XG_ST=0,YG_ST=0,ZG_ST=0, FS_SEL=0 -> ± 250 °/s
		Data = 0x00;
		HAL_I2C_Mem_Write(i2c, MPU6050_ADDR, GYRO_CONFIG_REG, 1, &Data, 1,
				1000);
		return true;
	}

	void UpdateAccel(void) {
		uint8_t Rec_Data[6];

		// Read 6 BYTES of data starting from ACCEL_XOUT_H register

		HAL_I2C_Mem_Read(i2c, MPU6050_ADDR, ACCEL_XOUT_H_REG, 1, Rec_Data, 6,
				1000);

		int16_t Accel_X_RAW = (int16_t) (Rec_Data[0] << 8 | Rec_Data[1]);
		int16_t Accel_Y_RAW = (int16_t) (Rec_Data[2] << 8 | Rec_Data[3]);
		int16_t Accel_Z_RAW = (int16_t) (Rec_Data[4] << 8 | Rec_Data[5]);

		/*** convert the RAW values into acceleration in 'g'
		 I have configured FS_SEL = 0. So I am dividing by 16384.0
		 for more details check ACCEL_CONFIG Register              ****/

		A[0] = Accel_X_RAW / 16384.0;
		A[1] = Accel_Y_RAW / 16384.0;
		A[2] = Accel_Z_RAW / 16384.0;
	}

	void UpdateGyro(void) {
		uint8_t Rec_Data[6];

		// Read 6 BYTES of data starting from GYRO_XOUT_H register

		HAL_I2C_Mem_Read(i2c, MPU6050_ADDR, GYRO_XOUT_H_REG, 1, Rec_Data, 6,
				1000);

		int16_t Gyro_X_RAW = (int16_t) (Rec_Data[0] << 8 | Rec_Data[1]);
		int16_t Gyro_Y_RAW = (int16_t) (Rec_Data[2] << 8 | Rec_Data[3]);
		int16_t Gyro_Z_RAW = (int16_t) (Rec_Data[4] << 8 | Rec_Data[5]);

		/*** convert the RAW values into dps (°/s)
		 we have to divide according to the Full scale value set in FS_SEL
		 I have configured FS_SEL = 0. So I am dividing by 131.0
		 for more details check GYRO_CONFIG Register              ****/

		G[0] = Gyro_X_RAW / 131.0;
		G[1] = Gyro_Y_RAW / 131.0;
		G[2] = Gyro_Z_RAW / 131.0;
	}

	float GetAx(){return A[0];}
	float GetAy(){return A[1];}
	float GetAz(){return A[2];}
	float GetGx(){return G[0];}
	float GetGy(){return G[1];}
	float GetGz(){return G[2];}

};
}

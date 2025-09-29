#include <Arduino.h>

/*
Ping() to test a servo on the bus is ready.
*/

// the UART used to control servos.
// GPIO 18 - S_RXD, GPIO 19 - S_TXD, as default.
#define S_RXD 18
#define S_TXD 19

#include <SCServo.h>

SCSCL sc;

int TEST_ID = 0xfe;

void setup()
{

  Serial.begin(115200);
  Serial1.begin(1000000, SERIAL_8N1, S_RXD, S_TXD);
  sc.pSerial = &Serial1;
  delay(1000);
}

void loop()
{
  sc.WritePos(0xfe, 1000, 0, 1500); // All servos move at the max speed=1500, move to Position=1000
  delay(754); // [(P1-P0)/V]*1000+100
  
  sc.WritePos(0xfe, 20, 0, 1500); // All servos move at the max speed=1500, move to Position=20
  delay(754); // [(P1-P0)/V]*1000+100
}
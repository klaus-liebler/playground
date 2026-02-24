#include <HardwareSerial.h>
#include <SCServo.h>
HardwareSerial Serial1(USART1, PC5, PC4, NC, NC); 
SMS_STS st;



void setup()
{
  Serial.begin(115200);
  Serial1.begin(1000000, SERIAL_8N1);
  st.pSerial = &Serial4;
  delay(1000);
}

void loop()
{
  st.WritePosEx(1, 4095, 3400, 50);//servo(ID1) speed=3400，acc=50，move to position=4095.
  delay(2000);
  
  st.WritePosEx(1, 2000, 1500, 50);//servo(ID1) speed=3400，acc=50，move to position=2000.
  delay(2000);
}

#define TINY_GSM_MODEM_SIM7080

// Set serial for debug console (to the Serial Monitor, default speed 115200)
#define SerialMon Serial

#define SerialAT Serial2

// Increase RX buffer to capture the entire response
// Chips without internal buffering (A6/A7, ESP8266, M590)
// need enough space in the buffer for the entire response
// else data will be lost (and the http library will fail).
#if !defined(TINY_GSM_RX_BUFFER)
#define TINY_GSM_RX_BUFFER 650
#endif

// See all AT commands, if wanted
//#define DUMP_AT_COMMANDS

// Define the serial console for debug prints, if needed
#define TINY_GSM_DEBUG SerialMon
// #define LOGGING  // <- Logging is for the HTTP library

// Range to attempt to autobaud
// NOTE:  DO NOT AUTOBAUD in production code.  Once you've established
// communication, set a fixed baud rate using modem.setBaud(#).
#define GSM_AUTOBAUD_MIN 9600
#define GSM_AUTOBAUD_MAX 115200
#define PIN_DTR 25
#define PIN_TX 27 // Microcontroller side
#define PIN_RX 26 // Microcontroller side
#define PIN_PWR 4 // GPIO pin used for PWRKEY
#define BAUD 115200

// Add a reception delay, if needed.
// This may be needed for a fast processor at a slow baud rate.
// #define TINY_GSM_YIELD() { delay(2); }

// Define how you're planning to connect to the internet
// These defines are only for this example; they are not needed in other code.
#define TINY_GSM_USE_GPRS true
#define TINY_GSM_USE_WIFI false

// set GSM PIN, if any
#define GSM_PIN "8505"

// Your GPRS credentials, if any
const char apn[] = "hologram";
const char gprsUser[] = "";
const char gprsPass[] = "";

// Server details
const char server[] = "vsh.pp.ua";
const char resource[] = "/TinyGSM/logo.txt";
const int port = 80;

#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>

// Just in case someone defined the wrong thing..
#if TINY_GSM_USE_GPRS && not defined TINY_GSM_MODEM_HAS_GPRS
#undef TINY_GSM_USE_GPRS
#undef TINY_GSM_USE_WIFI
#define TINY_GSM_USE_GPRS false
#define TINY_GSM_USE_WIFI true
#endif
#if TINY_GSM_USE_WIFI && not defined TINY_GSM_MODEM_HAS_WIFI
#undef TINY_GSM_USE_GPRS
#undef TINY_GSM_USE_WIFI
#define TINY_GSM_USE_GPRS true
#define TINY_GSM_USE_WIFI false
#endif

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif

TinyGsmClient client(modem);
HttpClient http(client, server, port);

const char *register_info[] = {
    "Not registered, MT is not currently searching an operator to register to.The GPRS service is disabled, the UE is allowed to attach for GPRS if requested by the user.",
    "Registered, home network.",
    "Not registered, but MT is currently trying to attach or searching an operator to register to. The GPRS service is enabled, but an allowable PLMN is currently not available. The UE will start a GPRS attach as soon as an allowable PLMN is available.",
    "Registration denied, The GPRS service is disabled, the UE is not allowed to attach for GPRS if it is requested by the user.",
    "Unknown.",
    "Registered, roaming.",
};

void setup()
{
  // Set console baud rate
  SerialMon.begin(115200);
  SerialAT.begin(BAUD, SERIAL_8N1, PIN_RX, PIN_TX);
  delay(10);

  pinMode(PIN_PWR, OUTPUT);
  pinMode(PIN_DTR, OUTPUT);

  SerialMon.println("===Start modem .");
  int retry = 0;
  while (!modem.testAT(1000))
  {
    Serial.print(".");
    if (retry++ > 6)
    {
      // Pull down PWRKEY for more than 1 second according to manual requirements
      digitalWrite(PIN_PWR, LOW);
      delay(100);
      digitalWrite(PIN_PWR, HIGH);
      delay(1000);
      digitalWrite(PIN_PWR, LOW);
      retry = 0;
      SerialMon.println("Retry start modem .");
    }
  }
  delay(5000);
  SerialMon.println();
  SerialMon.print("Modem started!");

  SerialMon.println("===Check the SIM status");
  String result;
  SimStatus ss = modem.getSimStatus();
  if (ss == SimStatus::SIM_ANTITHEFT_LOCKED)
  {
    SerialMon.println("SimStatus::SIM_ANTITHEFT_LOCKED!!!");
    return;
  }
  else if (ss == SimStatus::SIM_ERROR)
  {
    SerialMon.println("SimStatus::SIM_ERROR!!!");
    return;
  }
  else if (ss == SimStatus::SIM_LOCKED){
    SerialMon.println("SimStatus::SIM_LOCKED -->trying to unlock");
    modem.simUnlock(GSM_PIN);
    ss = modem.getSimStatus();
  }
  if (ss == SimStatus::SIM_READY)
  {
    SerialMon.println("SIM Card ready");
  }
  else
  {
    SerialMon.println("Unable to unlock sim");
  }

  SerialMon.println("===Disable RF");
  modem.sendAT("+CFUN=0");
  if (modem.waitResponse(20000UL) != 1)
  {
    SerialMon.println("Disable RF Failed!");
  }
  SerialMon.println("===Set Network and preferred mode");
  modem.setNetworkMode(2);   // use automatic
  modem.setPreferredMode(3); // 3 CAT-M and NB-IoT
  uint8_t pre = modem.getPreferredMode();
  uint8_t mode = modem.getNetworkMode();
  SerialMon.printf("getNetworkMode:%u getPreferredMode:%u\n", mode, pre);

  SerialMon.println("===Set the APN manually. Some operators need to set APN first when registering the network.");
  modem.sendAT("+CGDCONT=1,\"IP\",\"", apn, "\"");
  if (modem.waitResponse() != 1)
  {
    Serial.println("Set operators apn Failed!");
    return;
  }

  SerialMon.println("===Set the APN manually. Some operators need to set APN first when registering the network.");
  modem.sendAT("+CNCFG=0,1,\"", apn, "\"");
  if (modem.waitResponse() != 1)
  {
    Serial.println("Config apn Failed!");
    return;
  }

  SerialMon.println("===Enable RF");
  modem.sendAT("+CFUN=1");
  if (modem.waitResponse(20000UL) != 1)
  {
    Serial.println("Enable RF Failed!");
  }

  SerialMon.println("===Wait for the network registration to succeed");
  bool networkState = modem.waitForNetwork(60000);
  SerialMon.printf("Network result %s\n", networkState?"true":"false");


  SerialMon.println("===Activate network bearer, APN can not be configured by default,");
  modem.sendAT("+CNACT=0,1");
  if (modem.waitResponse() != 1)
  {
    Serial.println("Activate network bearer Failed!");
    return;
  }

  bool res = modem.isGprsConnected();
  Serial.print("GPRS status:");
  Serial.println(res ? "connected" : "not connected");

  String ccid = modem.getSimCCID();
  Serial.print("CCID:");
  Serial.println(ccid);

  String imei = modem.getIMEI();
  Serial.print("IMEI:");
  Serial.println(imei);

  String imsi = modem.getIMSI();
  Serial.print("IMSI:");
  Serial.println(imsi);

  String cop = modem.getOperator();
  Serial.print("Operator:");
  Serial.println(cop);

  IPAddress local = modem.localIP();
  Serial.print("Local IP:");
  Serial.println(local);

  int csq = modem.getSignalQuality();
  Serial.print("Signal quality:");
  Serial.println(csq);


  SerialMon.print(F("Performing HTTP GET request... "));
  int err = http.get(resource);
  if (err != 0) {
    SerialMon.println(F("failed to connect"));
    delay(10000);
    return;
  }

  int status = http.responseStatusCode();
  SerialMon.print(F("Response status code: "));
  SerialMon.println(status);
  if (!status) {
    delay(10000);
    return;
  }

  SerialMon.println(F("Response Headers:"));
  while (http.headerAvailable()) {
    String headerName  = http.readHeaderName();
    String headerValue = http.readHeaderValue();
    SerialMon.println("    " + headerName + " : " + headerValue);
  }

  int length = http.contentLength();
  if (length >= 0) {
    SerialMon.print(F("Content length is: "));
    SerialMon.println(length);
  }
  if (http.isResponseChunked()) {
    SerialMon.println(F("The response is chunked"));
  }

  String body = http.responseBody();
  SerialMon.println(F("Response:"));
  SerialMon.println(body);

  SerialMon.print(F("Body length is: "));
  SerialMon.println(body.length());

  // Shutdown

  http.stop();
  SerialMon.println(F("Server disconnected"));
  modem.gprsDisconnect();
  SerialMon.println(F("GPRS disconnected"));
}


void loop()
{
    while (SerialMon.available()) {
        SerialAT.write(SerialMon.read());
    }
    while (SerialAT.available()) {
        SerialMon.write(SerialAT.read());
    }
}
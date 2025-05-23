#define SerialMon Serial
#define SerialAT Serial1
#define GSM_PIN "8505"
#define UART_BAUD 115200
#define PIN_DTR 25
#define PIN_TX 27 //Microcontroller side
#define PIN_RX 26 //Microcontroller side
#define PWR_PIN 4
#define APN "web.vodafone.de"

#include "BotleticsSIM7000.h" // https://github.com/botletics/Botletics-SIM7000/tree/main/src
#define SIMCOM_7000
#define PWRKEY 4
#define RST 25 

#define TX 26 // Microcontroller RX
#define RX 27 // Microcontroller TX

 void printMenu(void);
 void flushSerial();
uint16_t readnumber();

// this is a large buffer for replies
char replybuffer[255];


HardwareSerial *modemSerial = &Serial2;

Botletics_modem_LTE modem = Botletics_modem_LTE();

uint8_t readline(char *buff, uint8_t maxbuff, uint16_t timeout = 0);
uint8_t type;
char imei[16] = {0}; // MUST use a 16 character buffer for IMEI!

void setup() {


  pinMode(RST, OUTPUT);
  digitalWrite(RST, HIGH); // Default state
  
  // Turn on the module by pulsing PWRKEY low for a little bit
  // This amount of time depends on the specific module that's used
  modem.powerOn(PWRKEY);

  Serial.begin(115200);
  Serial.println(F("SIM7XXX Demo"));

  // SIM7000 takes about 3s to turn on and SIM7500 takes about 15s
  // Press Arduino reset button if the module is still turning on and the board doesn't find it.
  // When the module is on it should communicate right after pressing reset

  // Software serial:
  modemSerial->begin(UART_BAUD, SERIAL_8N1, PIN_RX, PIN_TX);

  if (! modem.begin(*modemSerial)) {
    DEBUG_PRINTLN(F("Couldn't find SIM7000"));
  }
  

  type = modem.type();
  Serial.println(F("Modem is OK"));
  Serial.print(F("Found "));
  switch (type) {
    case SIM7000:
      Serial.println(F("SIM7000")); break;
    case SIM7070:
      Serial.println(F("SIM7070")); break;
    case SIM7500:
      Serial.println(F("SIM7500")); break;
    case SIM7600:
      Serial.println(F("SIM7600")); break;
    default:
      Serial.println(F("???")); break;
  }
  
  // Print module IMEI number.
  uint8_t imeiLen = modem.getIMEI(imei);
  if (imeiLen > 0) {
    Serial.print("Module IMEI: "); Serial.println(imei);
  }

  // Set modem to full functionality
  modem.setFunctionality(1); // AT+CFUN=1

  // Configure a GPRS APN, username, and password.
  // You might need to do this to access your network's GPRS/data
  // network.  Contact your provider for the exact APN, username,
  // and password values.  Username and password are optional and
  // can be removed, but APN is required.
  //modem.setNetworkSettings(F("your APN"), F("your username"), F("your password"));
  //modem.setNetworkSettings(F("m2m.com.attz")); // For AT&T IoT SIM card
  //modem.setNetworkSettings(F("telstra.internet")); // For Telstra (Australia) SIM card - CAT-M1 (Band 28)
  modem.setNetworkSettings(F("web.vodafone.de"));

  // Optionally configure HTTP gets to follow redirects over SSL.
  // Default is not to follow SSL redirects, however if you uncomment
  // the following line then redirects over SSL will be followed.
  //modem.setHTTPSRedirect(true);

  /*
  // Other examples of some things you can set:
  modem.setPreferredMode(38); // Use LTE only, not 2G
  modem.setPreferredLTEMode(1); // Use LTE CAT-M only, not NB-IoT
  modem.setOperatingBand("CAT-M", 12); // AT&T uses band 12
//  modem.setOperatingBand("CAT-M", 13); // Verizon uses band 13
  modem.enableRTC(true);
  
  modem.enableSleepMode(true);
  modem.set_eDRX(1, 4, "0010");
  modem.enablePSM(true);

  // Set the network status LED blinking pattern while connected to a network (see AT+SLEDS command)
  modem.setNetLED(true, 2, 64, 3000); // on/off, mode, timer_on, timer_off
  modem.setNetLED(false); // Disable network status LED
  */

  printMenu();
}

void printMenu(void) {
  Serial.println(F("-------------------------------------"));
  Serial.println(F("[?] Print this menu"));
  Serial.println(F("[a] Read the ADC, 0V-VBAT for SIM7000"));
  Serial.println(F("[b] Read supply voltage")); // Will also give battery % charged for LiPo battery
  Serial.println(F("[C] Read the SIM CCID"));
  Serial.println(F("[U] Unlock SIM with PIN code"));
  Serial.println(F("[i] Read signal strength (RSSI)"));
  Serial.println(F("[n] Get network status"));
  
#if defined(SIMCOM_7500) || defined(SIMCOM_7600)
  // Audio/Volume
  Serial.println(F("[v] Set audio Volume"));
  Serial.println(F("[V] Get volume"));
  Serial.println(F("[H] Set headphone audio"));
  Serial.println(F("[e] Set external audio"));
#endif  

  // Phone
  Serial.println(F("[c] Make phone Call"));
  Serial.println(F("[A] Get call status"));
  Serial.println(F("[h] Hang up phone"));
  Serial.println(F("[p] Pick up phone"));

  // SMS
  Serial.println(F("[N] Number of SMS's"));
  Serial.println(F("[r] Read SMS #"));
  Serial.println(F("[R] Read all SMS"));
  Serial.println(F("[d] Delete SMS #"));
  Serial.println(F("[D] Delete all SMS"));
  Serial.println(F("[s] Send SMS"));
  
  // Time
  Serial.println(F("[y] Enable local time stamp"));
  Serial.println(F("[Y] Enable NTP time sync")); // Need to use "G" command first!
  Serial.println(F("[t] Get network time")); // Works just by being connected to network

  // Data Connection
  Serial.println(F("[G] Enable GPRS"));
  Serial.println(F("[g] Disable GPRS"));
//  Serial.println(F("[w] Read webpage (GPRS)"));
//  Serial.println(F("[W] Post to website (GPRS)"));
  Serial.println(F("[1] Get connection info")); // See what connection type and band you're on!
  
  // The following option below posts dummy data to dweet.io for demonstration purposes. See the 
  // IoT_example sketch for an actual application of this function!
#if defined(SIMCOM_7000) || defined(SIMCOM_7070)
  Serial.println(F("[2] Post to dweet.io - LTE CAT-M / NB-IoT")); // SIM7000/7070
#elif defined(SIMCOM_7500) || defined(SIMCOM_7600)
  Serial.println(F("[3] Post to dweet.io - 4G LTE (SIM7X00)")); // SIM7500/7600
#endif

  // GPS
  Serial.println(F("[O] Turn GPS on)"));
  Serial.println(F("[o] Turn GPS off"));
  Serial.println(F("[L] Query GPS location"));
  
//  Serial.println(F("[S] Create serial passthru tunnel"));
  Serial.println(F("-------------------------------------"));
  Serial.println(F(""));
}

void loop() {
  Serial.print(F("modem> "));
  while (! Serial.available() ) {
    if (modem.available()) {
      Serial.write(modem.read());
    }
  }

  char command = Serial.read();
  Serial.println(command);


  switch (command) {
    case '?': {
        printMenu();
        break;
      }

    case 'a': {
        // read the ADC
        uint16_t adc;
        if (! modem.getADCVoltage(&adc)) {
          Serial.println(F("Failed to read ADC"));
        } else {
          Serial.print(F("ADC = ")); Serial.print(adc); Serial.println(F(" mV"));
        }
        break;
      }

    case 'b': {
        // Read the battery voltage and percentage
        uint16_t vbat;
        if (! modem.getBattVoltage(&vbat)) {
          Serial.println(F("Failed to read Batt"));
        } else {
          Serial.print(F("VBat = ")); Serial.print(vbat); Serial.println(F(" mV"));
        }

        if ( type != SIM7500 ) {
          if (! modem.getBattPercent(&vbat)) {
            Serial.println(F("Failed to read Batt"));
          } else {
            Serial.print(F("VPct = ")); Serial.print(vbat); Serial.println(F("%"));
          }
        }

        break;
      }

    case 'U': {
        // Unlock the SIM with a PIN code
        char PIN[5];
        flushSerial();
        Serial.println(F("Enter 4-digit PIN"));
        readline(PIN, 3);
        Serial.println(PIN);
        Serial.print(F("Unlocking SIM card: "));
        if (! modem.unlockSIM(PIN)) {
          Serial.println(F("Failed"));
        } else {
          Serial.println(F("OK!"));
        }
        break;
      }

    case 'C': {
        // read the CCID
        modem.getSIMCCID(replybuffer);  // make sure replybuffer is at least 21 bytes!
        Serial.print(F("SIM CCID = ")); Serial.println(replybuffer);
        break;
      }

    case 'i': {
        // read the RSSI
        uint8_t n = modem.getRSSI();
        int8_t r;

        Serial.print(F("RSSI = ")); Serial.print(n); Serial.print(": ");
        if (n == 0) r = -115;
        if (n == 1) r = -111;
        if (n == 31) r = -52;
        if ((n >= 2) && (n <= 30)) {
          r = map(n, 2, 30, -110, -54);
        }
        Serial.print(r); Serial.println(F(" dBm"));

        break;
      }

    case 'n': {
        // read the network/cellular status
        uint8_t n = modem.getNetworkStatus();
        Serial.print(F("Network status "));
        Serial.print(n);
        Serial.print(F(": "));
        if (n == 0) Serial.println(F("Not registered"));
        if (n == 1) Serial.println(F("Registered (home)"));
        if (n == 2) Serial.println(F("Not registered (searching)"));
        if (n == 3) Serial.println(F("Denied"));
        if (n == 4) Serial.println(F("Unknown"));
        if (n == 5) Serial.println(F("Registered roaming"));
        break;
      }

    /*** Audio ***/
#if defined(SIMCOM_7500) || defined(SIMCOM_7600)
    case 'v': {
        // set volume
        flushSerial();
        if ( (type == SIM5320A) || (type == SIM5320E) ) {
          Serial.print(F("Set Vol [0-8] "));
        } else if ( type == SIM7500 ) {
          Serial.print(F("Set Vol [0-5] "));
        } else {
          Serial.print(F("Set Vol % [0-100] "));
        }
        uint8_t vol = readnumber();
        Serial.println();
        if (! modem.setVolume(vol)) {
          Serial.println(F("Failed"));
        } else {
          Serial.println(F("OK!"));
        }
        break;
      }

    case 'V': {
        uint8_t v = modem.getVolume();
        Serial.print(v);
        if ( (type == SIM5320A) || (type == SIM5320E) ) {
          Serial.println(" / 8");
        } else {
          Serial.println("%");
        }
        break;
      }

    case 'H': {
        // Set Headphone output
        if (! modem.setAudio(HEADSETAUDIO)) {
          Serial.println(F("Failed"));
        } else {
          Serial.println(F("OK!"));
        }
        modem.setMicVolume(HEADSETAUDIO, 15);
        break;
      }
    case 'e': {
        // Set External output
        if (! modem.setAudio(EXTAUDIO)) {
          Serial.println(F("Failed"));
        } else {
          Serial.println(F("OK!"));
        }

        modem.setMicVolume(EXTAUDIO, 10);
        break;
      }

    /*** Call ***/
    case 'c': {
        // call a phone!
        char number[30];
        flushSerial();
        Serial.print(F("Call #"));
        readline(number, 30);
        Serial.println();
        Serial.print(F("Calling ")); Serial.println(number);
        if (!modem.callPhone(number)) {
          Serial.println(F("Failed"));
        } else {
          Serial.println(F("Sent!"));
        }

        break;
      }
    case 'A': {
        // get call status
        int8_t callstat = modem.getCallStatus();
        switch (callstat) {
          case 0: Serial.println(F("Ready")); break;
          case 1: Serial.println(F("Could not get status")); break;
          case 3: Serial.println(F("Ringing (incoming)")); break;
          case 4: Serial.println(F("Ringing/in progress (outgoing)")); break;
          default: Serial.println(F("Unknown")); break;
        }
        break;
      }
      
    case 'h': {
        // hang up!
        if (! modem.hangUp()) {
          Serial.println(F("Failed"));
        } else {
          Serial.println(F("OK!"));
        }
        break;
      }

    case 'p': {
        // pick up!
        if (! modem.pickUp()) {
          Serial.println(F("Failed"));
        } else {
          Serial.println(F("OK!"));
        }
        break;
      }
#endif

    /*** SMS ***/

    case 'N': {
        // read the number of SMS's!
        int8_t smsnum = modem.getNumSMS();
        if (smsnum < 0) {
          Serial.println(F("Could not read # SMS"));
        } else {
          Serial.print(smsnum);
          Serial.println(F(" SMS's on SIM card!"));
        }
        break;
      }
    case 'r': {
        // read an SMS
        flushSerial();
        Serial.print(F("Read #"));
        uint8_t smsn = readnumber();
        Serial.print(F("\n\rReading SMS #")); Serial.println(smsn);

        // Retrieve SMS sender address/phone number.
        if (! modem.getSMSSender(smsn, replybuffer, 250)) {
          Serial.println("Failed!");
          break;
        }
        Serial.print(F("FROM: ")); Serial.println(replybuffer);

        // Retrieve SMS value.
        uint16_t smslen;
        if (! modem.readSMS(smsn, replybuffer, 250, &smslen)) { // pass in buffer and max len!
          Serial.println("Failed!");
          break;
        }
        Serial.print(F("***** SMS #")); Serial.print(smsn);
        Serial.print(" ("); Serial.print(smslen); Serial.println(F(") bytes *****"));
        Serial.println(replybuffer);
        Serial.println(F("*****"));

        break;
      }
    case 'R': {
        // read all SMS
        int8_t smsnum = modem.getNumSMS();
        uint16_t smslen;
        int8_t smsn;

        if ( (type == SIM5320A) || (type == SIM5320E) ) {
          smsn = 0; // zero indexed
          smsnum--;
        } else {
          smsn = 1;  // 1 indexed
        }

        for ( ; smsn <= smsnum; smsn++) {
          Serial.print(F("\n\rReading SMS #")); Serial.println(smsn);
          if (!modem.readSMS(smsn, replybuffer, 250, &smslen)) {  // pass in buffer and max len!
            Serial.println(F("Failed!"));
            break;
          }
          // if the length is zero, its a special case where the index number is higher
          // so increase the max we'll look at!
          if (smslen == 0) {
            Serial.println(F("[empty slot]"));
            smsnum++;
            continue;
          }

          Serial.print(F("***** SMS #")); Serial.print(smsn);
          Serial.print(" ("); Serial.print(smslen); Serial.println(F(") bytes *****"));
          Serial.println(replybuffer);
          Serial.println(F("*****"));
        }
        break;
      }

    case 'd': {
        // delete an SMS
        flushSerial();
        Serial.print(F("Delete #"));
        uint8_t smsn = readnumber();

        Serial.print(F("\n\rDeleting SMS #")); Serial.println(smsn);
        if (modem.deleteSMS(smsn)) {
          Serial.println(F("OK!"));
        } else {
          Serial.println(F("Couldn't delete"));
        }
        break;
      }

    case 'D': {
        // Delete all SMS
        flushSerial();
        Serial.println(F("\n\rDeleting all SMS"));
        if (modem.deleteAllSMS()) {
          Serial.println(F("OK!"));
        } else {
          Serial.println(F("Couldn't delete"));
        }
        break;
      }

    case 's': {
        // send an SMS!
        char sendto[21], message[141];
        flushSerial();
        Serial.print(F("Send to #"));
        readline(sendto, 20);
        Serial.println(sendto);
        Serial.print(F("Type out one-line message (140 char): "));
        readline(message, 140);
        Serial.println(message);
        if (!modem.sendSMS(sendto, message)) {
          Serial.println(F("Failed"));
        } else {
          Serial.println(F("Sent!"));
        }

        break;
      }

    /*** Time ***/

    case 'y': {
        // enable network time sync
        if (!modem.enableRTC(true))
          Serial.println(F("Failed to enable"));
        break;
      }

    case 'Y': {
        // enable NTP time sync
        if (!modem.enableNTPTimeSync(true, F("pool.ntp.org")))
          Serial.println(F("Failed to enable"));
        break;
      }

    case 't': {
        // read the time
        char buffer[23];

        modem.getTime(buffer, 23);  // make sure replybuffer is at least 23 bytes!
        Serial.print(F("Time = ")); Serial.println(buffer);
        break;
      }


    /******* GPS *******/

    case 'o': {
        // turn GPS off
        if (!modem.enableGPS(false))
          Serial.println(F("Failed to turn off"));
        break;
      }
    case 'O': {
        // turn GPS on
        if (!modem.enableGPS(true))
          Serial.println(F("Failed to turn on"));
        break;
      }
    case 'L': {
        /*
        // Uncomment this block if all you want to see is the AT command response
        // check for GPS location
        char gpsdata[120];
        modem.getGPS(0, gpsdata, 120);
        if (type == SIM808_V1)
          Serial.println(F("Reply in format: mode,longitude,latitude,altitude,utctime(yyyymmddHHMMSS),ttff,satellites,speed,course"));
        else if ( (type == SIM5320A) || (type == SIM5320E) || (type == SIM7500) )
          Serial.println(F("Reply in format: [<lat>],[<N/S>],[<lon>],[<E/W>],[<date>],[<UTC time>(yyyymmddHHMMSS)],[<alt>],[<speed>],[<course>]"));
        else
          Serial.println(F("Reply in format: mode,fixstatus,utctime(yyyymmddHHMMSS),latitude,longitude,altitude,speed,course,fixmode,reserved1,HDOP,PDOP,VDOP,reserved2,view_satellites,used_satellites,reserved3,C/N0max,HPA,VPA"));
        
        Serial.println(gpsdata);

        break;
        */

        float latitude, longitude, speed_kph, heading, altitude;

        // Uncomment the definitions below and the line immediately below the definitions if you want
        // to parse UTC time data as well, or use the line below it if you don't care
//        float second;
//        uint16_t year;
//        uint8_t month, day, hour, minute;
//        if (modem.getGPS(&latitude, &longitude, &speed_kph, &heading, &altitude, &year, &month, &day, &hour, &minute, &second)) {
        if (modem.getGPS(&latitude, &longitude, &speed_kph, &heading, &altitude)) { // Use this line instead if you don't want UTC time
          Serial.println(F("---------------------"));
          Serial.print(F("Latitude: ")); Serial.println(latitude, 6);
          Serial.print(F("Longitude: ")); Serial.println(longitude, 6);
          Serial.print(F("Speed: ")); Serial.println(speed_kph);
          Serial.print(F("Heading: ")); Serial.println(heading);
          Serial.print(F("Altitude: ")); Serial.println(altitude);
          // Comment out the stuff below if you don't care about UTC time
          /*
          Serial.print(F("Year: ")); Serial.println(year);
          Serial.print(F("Month: ")); Serial.println(month);
          Serial.print(F("Day: ")); Serial.println(day);
          Serial.print(F("Hour: ")); Serial.println(hour);
          Serial.print(F("Minute: ")); Serial.println(minute);
          Serial.print(F("Second: ")); Serial.println(second);
          */
          Serial.println(F("---------------------"));
        }

        break;
      }

    /******* GPRS *******/

    case 'g': {
        // disable data
        if (!modem.enableGPRS(false))
          Serial.println(F("Failed to turn off"));
        break;
      }
    case 'G': {
        // turn GPRS off first for SIM7500
        #if defined(SIMCOM_7500) || defined (SIMCOM_7600)
          modem.enableGPRS(false);
        #endif
        
        // enable data
        if (!modem.enableGPRS(true))
            Serial.println(F("Failed to turn on"));
        break;
      }
    case '1': {
        // Get connection type, cellular band, carrier name, etc.
        modem.getNetworkInfo();        
        break;
      }

#if defined(SIMCOM_2G) || defined(SIMCOM_7000) || defined(SIMCOM_7070)
    case '2': {
        // Post data to website via 2G or LTE CAT-M/NB-IoT
        
        float temperature = analogRead(A0)*1.23; // Change this to suit your needs
        
        uint16_t battLevel;
        if (! modem.getBattVoltage(&battLevel)) battLevel = 3800; // Use dummy voltage if can't read

        // Create char buffers for the floating point numbers for sprintf
        // Make sure these buffers are long enough for your request URL
        char URL[150];
        char body[100];
        char tempBuff[16];
      
        // Format the floating point numbers as needed
        dtostrf(temperature, 1, 2, tempBuff); // float_val, min_width, digits_after_decimal, char_buffer

#ifdef SIMCOM_7070 // Use this line if you have the SIM7000G because the 1529B01SIM7000G firmware doesn't seem to run the commands below well
// #if defined(SIMCOM_7000) || defined(SIMCOM_7070) // Use this if you have SIM7000A, especially with SSL
            // Add headers as needed
            // modem.HTTP_addHeader("User-Agent", "SIM7070", 7);
            // modem.HTTP_addHeader("Cache-control", "no-cache", 8);
            // modem.HTTP_addHeader("Connection", "keep-alive", 10);
            // modem.HTTP_addHeader("Accept", "*/*, 3);
            
            // Connect to server
            // If https:// is used, #define BOTLETICS_SSL 1 in Botletics_modem.h
            if (! modem.HTTP_connect("http://dweet.io")) {
              Serial.println(F("Failed to connect to server..."));
              break;
            }

            // GET request
            // Format URI with GET request query string
            sprintf(URL, "/dweet/for/%s?temp=%s&batt=%i", imei, tempBuff, battLevel);
            modem.HTTP_GET(URL);

            // POST request
            /*
            sprintf(URL, "/dweet/for/%s", imei); // Format URI

            // Format JSON body for POST request
            // Example JSON body: "{\"temp\":\"22.3\",\"batt\":\"3800\"}"
            sprintf(body, "{\"temp\":\"%s\",\"batt\":\"%i\"}", tempBuff, battLevel); // construct JSON body

            modem.HTTP_addHeader("Content-Type", "application/json", 16);
            modem.HTTP_POST(URL, body, strlen(body));
            */

        #else
            // Construct the appropriate URL's and body, depending on request type
            // Use IMEI as device ID for this example
            
            // GET request
            sprintf(URL, "dweet.io/dweet/for/%s?temp=%s&batt=%i", imei, tempBuff, battLevel); // No need to specify http:// or https://
    //        sprintf(URL, "http://dweet.io/dweet/for/%s?temp=%s&batt=%i", imei, tempBuff, battLevel); // But this works too

            if (!modem.postData("GET", URL))
              Serial.println(F("Failed to complete HTTP GET..."));
            
            // POST request
            /*
            sprintf(URL, "http://dweet.io/dweet/for/%s", imei);
            sprintf(body, "{\"temp\":%s,\"batt\":%i}", tempBuff, battLevel);
            
            if (!modem.postData("POST", URL, body)) // Can also add authorization token parameter!
              Serial.println(F("Failed to complete HTTP POST..."));
            */
          
        #endif

        break;
      }
#endif
      
#if defined(SIMCOM_7500) || defined(SIMCOM_7600)
    case '3': {
        // Post data to website via 3G or 4G LTE
        float temperature = analogRead(A0)*1.23; // Change this to suit your needs
        
        uint16_t battLevel;
        if (! modem.getBattVoltage(&battLevel)) battLevel = 3800; // Use dummy voltage if can't read

        // Create char buffers for the floating point numbers for sprintf
        // Make sure these buffers are long enough for your request URL
        char URL[150];
        char tempBuff[12];
      
        // Format the floating point numbers as needed
        dtostrf(temperature, 1, 2, tempBuff); // float_val, min_width, digits_after_decimal, char_buffer

        // Construct the appropriate URL's and body, depending on request type
        // Use IMEI as device ID for this example
        
        // GET request
        sprintf(URL, "GET /dweet/for/%s?temp=%s&batt=%i HTTP/1.1\r\nHost: dweet.io\r\nContent-Length: 0\r\n\r\n", imei, tempBuff, battLevel);

        if (!modem.postData("www.dweet.io", 443, "HTTPS", URL)) // Server, port, connection type, URL
          Serial.println(F("Failed to complete HTTP/HTTPS request..."));
      
        break;
      }
#endif
    /*****************************************/

    case 'S': {
        Serial.println(F("Creating SERIAL TUBE"));
        while (1) {
          while (Serial.available()) {
            delay(1);
            modem.write(Serial.read());
          }
          if (modem.available()) {
            Serial.write(modem.read());
          }
        }
        break;
      }

    default: {
        Serial.println(F("Unknown command"));
        printMenu();
        break;
      }
  }
  // flush input
  flushSerial();
  while (modem.available()) {
    Serial.write(modem.read());
  }

}

void flushSerial() {
  while (Serial.available())
    Serial.read();
}

char readBlocking() {
  while (!Serial.available());
  return Serial.read();
}
uint16_t readnumber() {
  uint16_t x = 0;
  char c;
  while (! isdigit(c = readBlocking())) {
    //Serial.print(c);
  }
  Serial.print(c);
  x = c - '0';
  while (isdigit(c = readBlocking())) {
    Serial.print(c);
    x *= 10;
    x += c - '0';
  }
  return x;
}

uint8_t readline(char *buff, uint8_t maxbuff, uint16_t timeout) {
  uint16_t buffidx = 0;
  boolean timeoutvalid = true;
  if (timeout == 0) timeoutvalid = false;

  while (true) {
    if (buffidx > maxbuff) {
      //Serial.println(F("SPACE"));
      break;
    }

    while (Serial.available()) {
      char c =  Serial.read();

      //Serial.print(c, HEX); Serial.print("#"); Serial.println(c);

      if (c == '\r') continue;
      if (c == 0xA) {
        if (buffidx == 0)   // the first 0x0A is ignored
          continue;

        timeout = 0;         // the second 0x0A is the end of the line
        timeoutvalid = true;
        break;
      }
      buff[buffidx] = c;
      buffidx++;
    }

    if (timeoutvalid && timeout == 0) {
      //Serial.println(F("TIMEOUT"));
      break;
    }
    delay(1);
  }
  buff[buffidx] = 0;  // null term
  return buffidx;
}
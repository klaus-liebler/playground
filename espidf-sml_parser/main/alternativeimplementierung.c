#include <Arduino.h>
#include <AltSoftSerial.h>
#include <Wire.h>
#include <FastCRC.h>

#define LED 13
#define RAWPIN 6
#define HOTWIRE 2

// States for the loop() state machine
enum STATES : uint8_t { INIT=0, FIND_PAUSE, WAIT_DATA, IN_PACKET, TRAILER, READ_DATA, WRITE_REFINED, FINISH_WRITE };
STATES state = INIT;

AltSoftSerial D0IN;

FastCRC16 CRC16;

const uint16_t BUFFERSIZE(400);
uint8_t data[BUFFERSIZE];
uint16_t PACKETLENGTH;

// Class to hold OBIS byte sequences to process
enum O_TYPE : uint8_t { O_BYTE=0, RES1, RES2, RES3, O_BOOL, O_INT, O_UINT, O_LIST, O_FLOAT=0x7F };
const int SEQLEN(24);
class Sequence
{
public:
  bool complete;        // While parsing, set to true if the value was found
  bool inactive;        // Flag to premanently disable this sequence
  bool S_once;          // If found once, this sequence will be switched to inactive
  uint8_t S_ptr;        // While parsing, index in seq successfully recognized
  uint8_t S_len;        // Length of sequence
  uint16_t S_addr;      // Target address when writing I²C
  uint16_t S_tlen;      // maximum length of target data
  O_TYPE S_typ;         // Target data type
  uint8_t S_seq[SEQLEN];    // Sequence of bytes to be recognized

  Sequence(uint8_t l, uint16_t a, bool o1, O_TYPE t, uint16_t tl, uint8_t s[])
  {
    reset();
    inactive = false;
    S_len = (l>SEQLEN) ? SEQLEN : l;
    S_addr = a;
    S_once = o1;
    S_typ = t;
    S_tlen = tl;
    memset(S_seq, 0, SEQLEN);
    memcpy(S_seq, s, S_len);
  }
  ~Sequence() {}

  inline void reset() { S_ptr = 0; complete = false; }

  inline uint8_t check(uint8_t b)
  {
    if (b==S_seq[S_ptr])
    {
      if (S_ptr<S_len) S_ptr++;
    }
    else
    {
      S_ptr = 0;
    }
    return S_len-S_ptr;
  }
};

const uint8_t SEQUENCES(9);
Sequence sequences[SEQUENCES] =
{
  //       len, addr,  once,    type, tl, sequence
  Sequence(  8,    2, false, O_FLOAT,  4, (uint8_t[]){0x77, 0x07, 0x01, 0x00, 0x10, 0x07, 0x00, 0xFF } ),
  Sequence(  8,    6, false, O_FLOAT,  4, (uint8_t[]){0x77, 0x07, 0x01, 0x00, 0x01, 0x08, 0x00, 0xFF } ),
  Sequence(  8,   10, false, O_FLOAT,  4, (uint8_t[]){0x77, 0x07, 0x01, 0x00, 0x02, 0x08, 0x00, 0xFF } ),
  Sequence(  8,   14, false, O_FLOAT,  4, (uint8_t[]){0x77, 0x07, 0x01, 0x00, 0x01, 0x08, 0x01, 0xFF } ),
  Sequence(  8,   18, false, O_FLOAT,  4, (uint8_t[]){0x77, 0x07, 0x01, 0x00, 0x02, 0x08, 0x01, 0xFF } ),
  Sequence(  8,   22, false, O_FLOAT,  4, (uint8_t[]){0x77, 0x07, 0x01, 0x00, 0x01, 0x08, 0x02, 0xFF } ),
  Sequence(  8,   26, false, O_FLOAT,  4, (uint8_t[]){0x77, 0x07, 0x01, 0x00, 0x02, 0x08, 0x02, 0xFF } ),
  Sequence(  8,   30,  true,  O_BYTE,  4, (uint8_t[]){0x77, 0x07, 0x81, 0x81, 0xC7, 0x82, 0x03, 0xFF } ),
  Sequence(  8,   34,  true,  O_BYTE, 12, (uint8_t[]){0x77, 0x07, 0x01, 0x00, 0x00, 0x00, 0x09, 0xFF } ),
};

const unsigned char header[] = { 0x1B, 0x1B, 0x1B, 0x1B, 0x01, 0x01, 0x01, 0x01, 0x00 };
const unsigned char trailer[] = { 0x1B, 0x1B, 0x1B, 0x1B, 0x1A, 0x00 };

#ifdef TESTING
const uint16_t BUFLEN(64);
char buffer[BUFLEN];

// Helper function to generate a printable hexadecimal dump
// head: printed first
// sep:  separator char printed between each byte. Omitted, if ==0
// data: pointer to data to be dumped
// length: number of bytes in data
// buffer: target to be printed into
// max_buffer_length: obvious... ;)
uint16_t hexDump(const char *head, const char sep, uint8_t *data, uint16_t length, char *buffer, uint16_t max_buffer_length)
{
  uint16_t maxByte = 0;               // maximum byte to fit in buffer
  uint16_t outLen = 0;                // used length in buffer
  uint8_t byteLen = (sep?3:2);        // length of a single byte dumped
  uint16_t headLen = strlen(head);    // length of header
  const char *PRINTABLES = "0123456789ABCDEF";

  // if not even the header will fit into buffer, bail out.
  if (headLen>max_buffer_length) return 0;

  // if we have a header, print it.
  if (headLen) strcpy(buffer, head);
  outLen = headLen;

  // Calculate number of bytes fitting into remainder of buffer
  maxByte = (max_buffer_length - headLen) / byteLen;
  // If more than neede, reduce accordingly
  if (length<maxByte) maxByte = length;

  // May we print at least a single byte?
  if (maxByte>0)
  {
    // Yes. Start behind the header
    char *cp = buffer + headLen;
    // For each byte...
    for (uint16_t i=0; i<maxByte; ++i)
    {
      // ...print it, plus...
      *cp++ = PRINTABLES[(data[i]>>4)&0x0F];
      *cp++ = PRINTABLES[data[i]&0x0F];
      outLen += 2;
      // .. a separator, if it is defined and another byte is to follow
      if (sep && i<maxByte-1)
      {
        *cp++ = sep;
        outLen++;
      }
    }
    *cp = 0;
  }
  return outLen;
}
#endif

// Helper function to get type and (extended) length information
// bytePtr: pointer to TL byte in byte stream
// type: variable reference to return type information
// length: variable reference to return length information
// advanceBytes: number of bytes to skip to get to the value proper
// Function returns 0 if okay or !=0 to denote an error or unlikely data
//   0: OK
//   1: reserved data type - unlikely
//   2: impossible length
//   3: length bigger than expected
uint16_t getLengthType(uint8_t *bytePtr, O_TYPE& type, uint16_t& length, uint16_t& advanceBytes, uint16_t expectedLength)
{
  uint8_t *ptr = bytePtr;

  // At least one TL byte
  advanceBytes = 1;

  // Get data type
  type = (O_TYPE)(((*ptr)>>4) & 0x07);

  // Reserved? Unlikely...
  if (type==RES1 || type==RES2 || type==RES3) return 1;

  // Get (first) length nibble
  length = (*ptr)&0x0F;

  while (*ptr&0x80) // extended length byte
  {
    ptr++;
    length <<= 4;
    length |= (*ptr)&0x0F;
    advanceBytes++;
  }

  // length pointing behind complete packet? Impossible!
  if ((bytePtr-data)+length>PACKETLENGTH-8) return 2;

  length -= advanceBytes;

  // Length bigger than anticipated?
  if (length>expectedLength) return 3;

  // Looks okay.
  return 0;
}

void setup() {
  // Set up LED
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);

  // Set up data write enable pin
  pinMode(HOTWIRE, INPUT_PULLUP);

  // Set up raw data/refined data jumper pin
  pinMode(RAWPIN, INPUT_PULLUP);

  // Start Serial interface for photo sensor
  D0IN.begin(9600);

  // Start I²C
  Wire.begin();
  Wire.setClock(400000L);

  // Ínit read buffer
  memset(data, 0, BUFFERSIZE);
  state = INIT;

#ifdef TESTING
  // Init Serial - in case we are doing test outputs
  Serial.begin(115200);
  Serial.println("");
#endif
}

// Helper function to write I²C packets
// addr - memory address on target device
// data - pointer to data to write
// len - number of bytes (must not exceed 30!)
// Returns written length
inline uint8_t writeI2C(uint16_t addr, uint8_t *data, uint8_t len)
{
  uint8_t length = (len>30)? 30 : len;

  Wire.beginTransmission(2);  // Neighbour device #2
  // Write address
  Wire.write((addr>>8)&0xFF);
  Wire.write(addr&0xFF);
  // Write packet
  Wire.write(data, length);
  // Connection is kept open!
  Wire.endTransmission();
  delay(2);
  return length;
}

// Helper function to check the HOTWIRE pin
inline int waitForWrite(uint16_t pin)
{
  uint32_t stop = millis();

  while (digitalRead(pin)==LOW)
  {
    if (millis()-stop>=5000) return 1;
  }
  return 0;
}

void loop()
{
  static uint8_t *cp = data;
  static uint8_t Startseq = 0;
  static uint8_t Endseq = 0;
  static uint8_t Trailerbytes = 0;
  static bool byteWaiting = false;
  static uint8_t byteRead = 0;
  const uint32_t PAUSETIME(200);
  static uint32_t pause = millis()+PAUSETIME;
  static uint16_t cnt = 0;
  static uint8_t rc = 0;
  static uint8_t field = 0;
  static uint32_t packetOK = 0;
  static uint32_t packetERR = 0;

#ifdef TESTING
  static bool TRACE_ON = false;

  // Is trace output required?
  if (digitalRead(RAWPIN)==LOW)
  {
    TRACE_ON = true;
  }
  else
  {
    TRACE_ON = false;
  }
#endif
  
  // May we read another byte?
  if (!byteWaiting && D0IN.available())
  {
    // Yes. Do it.
    byteRead = D0IN.read();
    byteWaiting = true;
  }

  switch (state)
  {
  case INIT:
    state = FIND_PAUSE;
    break;
  case FIND_PAUSE:
    // Detect pause
    if (!byteWaiting)
    {
      // Did we pass the pause time?
      if (millis()>pause)
      {
        // Yes. Go on catch a packet.
        Startseq = 0;
        cp = data;
        cnt = 0;
        state = WAIT_DATA;
      }
    }
    else
    {
      byteWaiting = false;
      pause = millis() + PAUSETIME;
    }
    break;
  case WAIT_DATA:
    // Pause found. Now find data packet
    if (byteWaiting)
    {
      // Is it the next in the header sequence?
      if (byteRead==header[Startseq])
      {
        // Yes. collect it and wait for the next
        *cp++ = byteRead;
        cnt++;
        // Overflow?
        if (cnt>=BUFFERSIZE)
        {
          // Yes. Reset data and start over
          state = FIND_PAUSE;
        }
        else
        {
          Startseq++;
          // ...unless it was the 4th. Then we have found the start sequence and may go on
          if (header[Startseq]==0x00)
          {
            Endseq = 0;
            state = IN_PACKET;
            digitalWrite(LED, HIGH);
          }
        }
      }
      else
      {
        // No - garbage or defective packet. Reset to begin the search again
        cnt = 0;
        cp = data;
        Startseq = 0;
      }
      byteWaiting = false;
    }
    break;
  case IN_PACKET:
    // Start sequence was there - read on
    if (byteWaiting)
    {
      *cp++ = byteRead;
      cnt++;
      // Overflow?
      if (cnt>=BUFFERSIZE)
      {
        // Yes. Reset data and start over
        digitalWrite(LED, LOW);
        state = FIND_PAUSE;
      }
      else
      {
        // Is it part of the ending sequence?
        if (byteRead==trailer[Endseq])
        {
          // Yes. count it.
          Endseq++;
          // If we had 4 in a row, proceed to catch the trailer
          if (trailer[Endseq]==0x00)
          {
            Trailerbytes = 0;
            state = TRAILER;
          }
        }
        else
        {
          // No - something else - discard count
          Endseq = 0;
        }
      }
      byteWaiting = false;
    }
    break;
  case TRAILER:
    // Get the final 3 bytes
    if (byteWaiting)
    {
      // Catch it.
      *cp++ = byteRead;
      cnt++;
      // Overflow?
      if (cnt>=BUFFERSIZE)
      {
        // Yes. Reset data and start over
        digitalWrite(LED, LOW);
        state = FIND_PAUSE;
      }
      else
      {
        Trailerbytes++;
        if (Trailerbytes==3)
        {
          state = READ_DATA;
        }
      }
      byteWaiting = false;
    }
    break;
  case READ_DATA:
    // Did we get some?
    if (cnt)
    {
      PACKETLENGTH = cnt;
      // Calculate CRC
      uint16_t crc = CRC16.x25(data, PACKETLENGTH-2);
      // Get CRC from packet
      uint16_t packetCRC = (data[PACKETLENGTH-1]<<8)|data[PACKETLENGTH-2];
#ifdef TESTING
      if (TRACE_ON)
      {
        uint16_t pos = 0;
        Serial.println("D0 data:");
        while (pos<cnt)
        {
          hexDump("  ", ' ', data+pos, (cnt-pos)<16 ? (cnt-pos) : 16, buffer, BUFLEN);
          Serial.println(buffer);
          pos += 16;
        }
        Serial.print("CRC-X25: ");
        Serial.print(packetCRC, HEX);
        Serial.print("<-->");
        Serial.println(crc, HEX);
      }
#endif
      // Matching CRC? If so, proceed to processing. Else discard packet.
      if (packetCRC==crc)
      {
        packetOK++;
        state = WRITE_REFINED;
      }
      else
      {
#ifdef TESTING
        if (TRACE_ON)
        {
          Serial.println("CRC err - discarded.");
        }
#endif
        packetERR++;
        digitalWrite(LED, LOW);
        state = FIND_PAUSE;
      }
    }
    else
    {
      // No. Wait for another packet
      digitalWrite(LED, LOW);
      state = FIND_PAUSE;
    }
    break;
  case WRITE_REFINED:
    {
      // Yes.
      cp = data;
      // Data proper
      uint16_t lcnt = 0;
      for (uint8_t i=0; i<SEQUENCES; i++)
      {
        sequences[i].reset();
      }
      while (rc==0 && lcnt<cnt)
      {
        // Check all sequences for a match
        for (uint8_t i=0; i<SEQUENCES; ++i)
        {
          if (sequences[i].complete || sequences[i].inactive) continue;
          // Sequence completely matching?
          if (sequences[i].check(*(cp+lcnt))==0)
          {
            // Yes, found it.
            // We will have a sequence of data fields to process:
            // - status
            // - valTime
            // - unit code
            // - scaler
            // - value
            uint8_t *cq = cp + lcnt + 1;
            uint16_t skipLen = 0;
            int scaler = 0;
            O_TYPE type;
            uint16_t length;
            uint16_t stepV;
            
            rc = 0;
            field = 0;

#ifdef TESTING
            if (TRACE_ON)
            {
              hexDump("Seq.found:", ' ', sequences[i].S_seq, sequences[i].S_len, buffer, BUFLEN);
              Serial.println(buffer);
            }
#endif

            // status: skip
            if ((rc = getLengthType(cq, type, length, stepV, 4)))
            {
#ifdef TESTING
              if (TRACE_ON)
              {
                Serial.println("status fld. err ");
                Serial.println(rc);
              }
#endif
              field = 1;
              break;
            }
            cq += stepV + length;
            skipLen += stepV + length;

            // valTime: skip
            if ((rc = getLengthType(cq, type, length, stepV, 8)))
            {
#ifdef TESTING
              if (TRACE_ON)
              {
                Serial.println("valTime fld. err ");
                Serial.println(rc);
              }
#endif
              field = 2;
              break;
            }
            cq += stepV + length;
            skipLen += stepV + length;

            // unit code: skip
            if ((rc = getLengthType(cq, type, length, stepV, 2)))
            {
#ifdef TESTING
              if (TRACE_ON)
              {
                Serial.println("unit fld. err ");
                Serial.println(rc);
              }
#endif
              field = 3;
              break;
            }
            cq += stepV + length;
            skipLen += stepV + length;

            // scaler: we will need it for O_FLOAT target values
            if ((rc = getLengthType(cq, type, length, stepV, 2)))
            {
#ifdef TESTING
              if (TRACE_ON)
              {
                Serial.println("scaler fld. err ");
                Serial.println(rc);
              }
#endif
              field = 4;
              break;
            }
            if (length==1) scaler = (signed char)*(cq+stepV);
            cq += stepV + length;
            skipLen += stepV + length;

            // value: depending on type, different treatments necessary
            if ((rc = getLengthType(cq, type, length, stepV, 100)))
            {
#ifdef TESTING
              if (TRACE_ON)
              {
                Serial.println("value fld. err ");
                Serial.println(rc);
              }
#endif
              field = 5;
              break;
            }

#ifdef TESTING
            if (TRACE_ON)
            {
              hexDump("Value:", ' ', cq, stepV+length, buffer, BUFLEN);
              Serial.println(buffer);
            }
#endif

            cq += stepV;
            // Just in case - we must have at least one byte
            if (length>0)
            {
              bool sign = false;
              switch (type)
              {
              case O_BYTE:
                if (length>sequences[i].S_tlen)
                {
                  waitForWrite(HOTWIRE);
                  writeI2C(sequences[i].S_addr, cq, sequences[i].S_tlen);
                }
                else
                {
                  waitForWrite(HOTWIRE);
                  writeI2C(sequences[i].S_addr, cq, length);
                }
                break;
              case O_BOOL:
                waitForWrite(HOTWIRE);
                writeI2C(sequences[i].S_addr, cq, 1);
                break;
              case O_INT:
                sign = true;
              case O_UINT:
                // Conversion into float required?
                if (sequences[i].S_typ==O_FLOAT)
                {
                  // Yes. Beware: Arduino floats and longs are 32 bits maximum!
                  // OBIS values can be longer, so we need to truncate or scale down.
                  // First we try to get the sign of the number
                  float Minus = 1.0;
                  // Can we expect negative values (type O_INT)?
                  if (sign)
                  {
                    // Yes. is the very first bit set?
                    if ((*cq&0x80))
                    {
                      // Yes, we have a negative number here
                      Minus = -1.0;
                    }
                    else
                    {
                      // No, positive - we may treat it like an unsigned.
                      sign = false;
                    }
                  }
                  float f = 0.0;
                  for (uint8_t j=0; j<length; ++j)
                  {
                    f *=256.0;
                    if (sign)
                    {
                      f += (~(*(cq+j)))&0xFF;
                    }
                    else
                    {
                      f += *(cq+j);
                    }
                  }
                  if (sign) f += 1.0;
                  f *= Minus;
                  // apply the scaler
                  if (scaler>0)
                  {
                    for (int j=scaler;j;--j) f *= 10.0;
                  }
                  else if (scaler<0)
                  {
                    for (int j=scaler;j;++j) f /= 10.0;
                  }
                  // Write 4 bytes of float in MSB order
                  uint32_t uf = *(uint32_t *)&f;  // Ugly... Not portable, but unavoidable
                  uint8_t wd[4];
                  wd[0] = (uf>>24) & 0xFF;
                  wd[1] = (uf>>16) & 0xFF;
                  wd[2] = (uf>>8) & 0xFF;
                  wd[3] = uf & 0xFF;
                  // Write it to memory, if allowed
                  waitForWrite(HOTWIRE);
                  writeI2C(sequences[i].S_addr, wd, 4);

#ifdef TESTING
                  if (TRACE_ON)
                  {
                    Serial.print("float value=");
                    Serial.println(f);
                    Serial.flush();
                  }
#endif

                }
                else
                {
                  if (length>sequences[i].S_tlen)
                  {
                    waitForWrite(HOTWIRE);
                    writeI2C(sequences[i].S_addr, cq, sequences[i].S_tlen);
                  }
                  else
                  {
                    waitForWrite(HOTWIRE);
                    writeI2C(sequences[i].S_addr, cq, length);
                  }
                }
                break;
              default: // All else does not interest us
                break;
              }
            }
            cq += length;
            skipLen += stepV + length;

            // signature: skip
            if ((rc = getLengthType(cq, type, length, stepV, 32)))
            {
#ifdef TESTING
              if (TRACE_ON)
              {
                Serial.println("signature fld. err ");
                Serial.println(rc);
              }
#endif
              field = 6;
              break;
            }
            cq += stepV + length;
            skipLen += stepV + length;

            // disable found sequence for this turn
            sequences[i].complete = true;
            if (sequences[i].S_once) sequences[i].inactive = true;
            lcnt += skipLen - 1;
            break;
          }
        }
        lcnt++;
      }
      state = FINISH_WRITE;
    }
    break;
  case FINISH_WRITE:
    {
      // Write read length, to address 0x0000
      uint8_t buffer[12];
      buffer[0] = 0;
      buffer[1] = 0;
      buffer[2] = (PACKETLENGTH>>8)&0xFF;
      buffer[3] = PACKETLENGTH&0xff;
      waitForWrite(HOTWIRE);
      Wire.beginTransmission(2);
      Wire.write(buffer, 4);
      Wire.endTransmission();
      delay(2);

      // Write return code, error field indicator and packet counts, to address 0x002E
      buffer[0] = 0;
      buffer[1] = 46;

      // Field indicator - >0, if field in error
      buffer[2] = field&0xFF;
      // Return code from field length/type detection
      buffer[3] = rc&0xff;
      // Number of intact packets processed
      buffer[4] = (packetOK>>24)&0xFF;
      buffer[5] = (packetOK>>16)&0xFF;
      buffer[6] = (packetOK>>8)&0xFF;
      buffer[7] = packetOK&0xFF;
      // Number of defective packets discarded
      buffer[8] = (packetERR>>24)&0xFF;
      buffer[9] = (packetERR>>16)&0xFF;
      buffer[10] = (packetERR>>8)&0xFF;
      buffer[11] = packetERR&0xFF;
      waitForWrite(HOTWIRE);
      Wire.beginTransmission(2);
      Wire.write(buffer, 12);
      Wire.endTransmission();
      delay(2);

      digitalWrite(LED, LOW);
      // Go fetch next
      state = FIND_PAUSE;
    }
    break;
  };
}
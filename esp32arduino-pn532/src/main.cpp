
#include "emulatetag.h"
#include "NdefMessage.h"


#include <PN532_HSU.h>
#include <PN532.h>

PN532_HSU pn532hsu(Serial2);
EmulateTag nfc(pn532hsu);

uint8_t ndefBuf[2048];
NdefMessage message;
int messageSize;

uint8_t uid[3] = {0x12, 0x34, 0x56};

#define RXD2 16
#define TXD2 17



void setup(void) {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
  Serial.println("------- Emulate Tag --------");

  message = NdefMessage();
  message.addUriRecord("https://google.com");
  message.addMimeMediaRecord("text/html", "<!doctype html><meta charset=utf-8><title>shortest html5</title>");
  messageSize = message.getEncodedSize();
  if (messageSize > sizeof(ndefBuf))
  {
    Serial.println("ndefBuf is too small");
    while (1)
    {
    }
  }

  Serial.print("Ndef encoded message size: ");
  Serial.println(messageSize);

  message.encode(ndefBuf);

  // comment out this command for no ndef message
  nfc.setNdefFile(ndefBuf, messageSize);

  // uid must be 3 bytes!
  nfc.setUid(uid);

  nfc.init();
}


void loop(void) {
  // uncomment for overriding ndef in case a write to this tag occured
  //nfc.setNdefFile(ndefBuf, messageSize);

  // start emulation (blocks)
  nfc.emulate();

  // or start emulation with timeout
  /*if(!nfc.emulate(1000)){ // timeout 1 second
      Serial.println("timed out");
    }*/

  // deny writing to the tag
  // nfc.setTagWriteable(false);

  if (nfc.writeOccured())
  {
    Serial.println("\nWrite occured !");
    uint8_t *tag_buf;
    uint16_t length;

    nfc.getContent(&tag_buf, &length);
    NdefMessage msg = NdefMessage(tag_buf, length);
    msg.print();
  }

  delay(1000);
}

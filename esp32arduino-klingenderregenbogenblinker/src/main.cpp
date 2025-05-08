#include <Arduino.h>
#include "ringtoneplayer.hh"
#include "rgbled.hh"

// Diverse konstante Werte werden gesetzt
constexpr gpio_num_t PIN_LED_WS2812 = GPIO_NUM_15;
constexpr gpio_num_t PIN_ONEWIRE = GPIO_NUM_14;
constexpr gpio_num_t PIN_LDR = GPIO_NUM_39;
constexpr gpio_num_t PIN_SPEAKER = GPIO_NUM_25;

constexpr size_t LED_NUMBER = 4;
constexpr uint32_t TIMEOUT_FOR_LED_STRIP = 1000;

constexpr uint32_t KLICK_KLACK_TIME_MS{500};

// Managementobjekt für die RGB-LEDs
RGBLED<LED_NUMBER, DeviceType::WS2812> *strip{nullptr};
Ringtoneplayer *ringtone{nullptr};


void setup() {
  // Richtet serielle Kommunikationsschnittstelle ein, damit die ganzen Meldungen am PC angezeigt werden können
  Serial.begin(115200);
  printf("W-HS IoT BeHampel/n");

   // Konfiguriert den 4fach-RGB-LED-Strip
  strip = new RGBLED<LED_NUMBER, DeviceType::WS2812>();
  ESP_ERROR_CHECK(strip->Init(VSPI_HOST, PIN_LED_WS2812, 2));
  ESP_ERROR_CHECK(strip->Clear(TIMEOUT_FOR_LED_STRIP));
  strip->SetPixel(0, CRGB::Red);
  strip->SetPixel(1, CRGB::Green);
  strip->SetPixel(2, CRGB::Blue);
  strip->SetPixel(3, CRGB::Yellow);
  strip->Refresh(TIMEOUT_FOR_LED_STRIP);

  //KOnfiguriert den Piepser
  ringtone = new Ringtoneplayer();
  ringtone->Setup(LEDC_TIMER_2, LEDC_CHANNEL_2, PIN_SPEAKER);
  ringtone->PlaySong(1);
}


uint32_t lastChange{0};
bool state{false};

void loop() {
  ringtone->Loop();
  uint32_t now = millis();
  if(now>=lastChange+500){
    if(state==true){
      strip->SetPixel(0, CRGB::Red);
      printf("Klick!\n");
    }
    else{
      strip->SetPixel(0, CRGB::Black);
      printf("Klack!\n");
    }
    strip->Refresh();
    state=!state;
    lastChange=now;
  }
  delay(50);
}
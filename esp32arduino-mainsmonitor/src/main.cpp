#include <stdio.h>
#include "esp_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/periph_ctrl.h"
#include "driver/timer.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <WebServer.h>
#include <Arduino.h>
#include <Wire.h>

#include "secrets.hh"

const uint16_t TIMER_DIVIDER = 80;   //  Hardware timer clock divider
const int64_t TIMER_ALARM = 4500000; //Wenn 45ms kein Impuls kam, dann ging etwas schief
//"Datenmodell" durch einfache globale Variablen
float frequency{0.0};
uint32_t impulses{0};
bool mains_loss{false};

char jsonBuffer[300];
const gpio_num_t PIN_MAINS = GPIO_NUM_0;
const timer_group_t MAINS_TIMER_GROUP = TIMER_GROUP_0;
const timer_idx_t MAINS_TIMER = TIMER_1;

static TaskHandle_t xTaskToNotify = NULL;
const UBaseType_t xArrayIndex = 1;

//Kommunikationsobjekt für WLAN
WiFiClient wifiClient;

//Kommunikationsobjekt für MQTT; nutzt WLAN
PubSubClient mqttClient(wifiClient);

//Kommunikationsobjekt Webserver
WebServer httpServer(80);

//Funktion erzeugt dynamisches HTML - letztlich die Website, die darzustellen ist
void handle_OnConnect()
{
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr += "<title>BeHampel</title>\n";
  ptr += "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr += "body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;}\n";
  ptr += "p {font-size: 24px;color: #444444;margin-bottom: 10px;}\n";
  ptr += "</style>\n";
  ptr += "</head>\n";
  ptr += "<body>\n";
  ptr += "<div id=\"webpage\">\n";
  ptr += "<h1>BeHampel</h1>\n";
  ptr += "<p>Frequency: ";
  ptr += frequency;
  ptr += "Hz</p>";
  ptr += "<p>Impulses: ";
  ptr += impulses;
  ptr += "</p>";
  ptr += "</div>\n";
  ptr += "</body>\n";
  ptr += "</html>\n";
  httpServer.send(200, "text/html", ptr);
}

//Generiert eine einfache Fehlerseite
void handle_NotFound()
{
  httpServer.send(404, "text/plain", "Not found");
}

//Funktion wird immer dann aufgerufen, wenn lab@home eine Nachricht über MQTT erhält - wird hier eigentlich nicht benötigt und ist nur "der Vollständigkeit halber" implementiert
void mqttCallback(char *topic, byte *message, unsigned int length)
{
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.println();
}

extern "C" void IRAM_ATTR gpio_isr_handler(void *arg)
{
  uint64_t currTimer{0};
  //Direct register access!!
  timer_get_counter_value(MAINS_TIMER_GROUP, MAINS_TIMER, &currTimer);
  frequency = 1000000.0 / (float)currTimer;
  timer_set_counter_value(MAINS_TIMER_GROUP, MAINS_TIMER, 0);
}

extern "C" void IRAM_ATTR mains_timer_isr(void *args)
{
  timer_spinlock_take(MAINS_TIMER_GROUP);

  /* Retrieve the interrupt status and the counter value
       from the timer that reported the interrupt */
  uint64_t timer_counter_value = timer_group_get_counter_value_in_isr(MAINS_TIMER_GROUP, MAINS_TIMER);

  timer_group_clr_intr_status_in_isr(MAINS_TIMER_GROUP, MAINS_TIMER);
  timer_group_set_alarm_value_in_isr(MAINS_TIMER_GROUP, timer_idx, timer_counter_value);

  /* After the alarm has been triggered
      we need enable it again, so it is triggered the next time */
  timer_group_enable_alarm_in_isr(MAINS_TIMER_GROUP, MAINS_TIMER);
  configASSERT(xTaskToNotify != NULL);
  /* Now just send the event data back to the main program task */
  vTaskNotifyGiveFromISR(xTaskToNotify, &xHigherPriorityTaskWoken);
  timer_spinlock_give(MAINS_TIMER_GROUP);
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  mains_loss = true;
  //xQueueSendFromISR(s_timer_queue, &evt, &high_task_awoken);
}

extern "C" void picpUPS(void *pvParameters)
{

  /* Select and initialize basic parameters of the timer */
  timer_config_t config = {};
  config.divider = TIMER_DIVIDER;
  config.counter_dir = TIMER_COUNT_UP;
  config.counter_en = TIMER_PAUSE;
  config.alarm_en = TIMER_ALARM_EN;
  config.auto_reload = false;
  timer_init(MAINS_TIMER_GROUP, MAINS_TIMER, &config);

  timer_set_counter_value(MAINS_TIMER_GROUP, MAINS_TIMER, 0);

  timer_set_alarm_value(MAINS_TIMER_GROUP, MAINS_TIMER, TIMER_ALARM);
  timer_enable_intr(MAINS_TIMER_GROUP, MAINS_TIMER);

  timer_isr_register(MAINS_TIMER_GROUP, MAINS_TIMER, mains_timer_isr, (void *)0, ESP_INTR_FLAG_IRAM, NULL);

  timer_start(MAINS_TIMER_GROUP, MAINS_TIMER);
  configASSERT(xTaskToNotify == NULL);

  /* Store the handle of the calling task. */
  xTaskToNotify = xTaskGetCurrentTaskHandle();
  while (true)
  {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    snprintf(jsonBuffer, sizeof(jsonBuffer), "{\"mainsloss\":%d}", millis());
    Serial.println(jsonBuffer);
    //mqttClient.publish(MQTT_TOPIC, jsonBuffer);
  }
}

void gpio_init()
{
  gpio_config_t io_conf = {};
  io_conf.intr_type = GPIO_INTR_POSEDGE;
  io_conf.mode = GPIO_MODE_INPUT;
  io_conf.pin_bit_mask = (1ULL << (int)PIN_MAINS);
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
  gpio_config(&io_conf);
  gpio_install_isr_service(0);
  gpio_isr_handler_add(PIN_MAINS, gpio_isr_handler, NULL);
}

//Funktion wird automatisch vom Framework einmalig beim einschalten bzw nach Reset aufgerufen
void setup()
{
  //Richtet serielle Kommunikationsschnittstelle ein, damit die ganzen Meldungen am PC angezeigt werden können
  Serial.begin(115200);
  Serial.println("Frequency Counter");

  //Legt fest, über welche Pins die sog. I2C-Schnittstelle zur Anbindung der beiden verwendete Sensoren laufen soll
  Wire.begin(22, 21);

  //Baut die Verbindung mit dem WLAN auf
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");

  //Es wird hinterlegt, welche Funktionen aufzurufen sind, wenn ein Browser sich verbindet
  //Wenn die "Hauptseite", also einfach "/" aufgerufen wird, dann soll handle_OnConnect aufgerufen werden
  httpServer.on("/", handle_OnConnect);
  //wenn etwas anderes aufgerufen wird, dann soll eine einfache Fehlerseite dargestellt werden
  httpServer.onNotFound(handle_NotFound);

  //Ab der nächsten Zeile ist der ESP32 für einen Webbrowser erreichbar, weil der sog "HTTP-Server" gestartet wird
  httpServer.begin();
  Serial.print("HTTP server started. Open http://");
  Serial.print(WiFi.localIP());
  Serial.println(" in your browser");

  //Baut die Verbindung zum MQTT-Server auf
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);
  if (!mqttClient.connect("freq-lab@home", MQTT_USER, MQTT_PASS))
  {
    Serial.print("MQTT connection failed. Freezing...");
    while (1)
      ;
  }
  gpio_init();
  TaskHandle_t xHandle = NULL;
  xTaskCreate(picpUPS, "picpUPS", 4 * 4096, NULL, 255, &xHandle);
  assert(xHandle);
}

uint32_t lastSensorUpdate = 0;
uint32_t lastMQTTUpdate = 0;

//Diese Funktion wird vom Framework immer und immer wieder aufgerufen
void loop()
{

  mqttClient.loop();
  httpServer.handleClient();

  int now = millis();

  if (now - lastSensorUpdate > 5000)
  {

    snprintf(jsonBuffer, sizeof(jsonBuffer), "{\"frequency\":%f}", frequency);
    Serial.println(jsonBuffer);

    lastSensorUpdate = now;
  }

  //Schreibe alle 20 Sekunden die aktuellen Messwerte per MQTT raus
  if (now - lastMQTTUpdate > 20000)
  {
    if (mqttClient.publish(MQTT_TOPIC, jsonBuffer))
    {
      Serial.println("Successfully published to MQTT Broker");
    }
    else
    {
      Serial.println("Error while publishing to MQTT Broker");
    }
    lastMQTTUpdate = now;
  }
}
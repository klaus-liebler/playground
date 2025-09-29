
#pragma once
#include <stdio.h>
#include "esp_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/periph_ctrl.h"
#include "driver/timer.h"
#include "driver/gpio.h"
#include "esp_partition.h"
#include "esp_log.h"

#define TAG "PUPS"

namespace picoups_config
{
  constexpr gpio_num_t PIN_MAINS = GPIO_NUM_36;
  constexpr timer_group_t MAINS_TIMER_GROUP = TIMER_GROUP_0;
  timg_dev_t *MAINS_TIMER_GROUP_RAW = &TIMERG0;
  constexpr timer_idx_t MAINS_TIMER = TIMER_1;
}

namespace picoups
{
  constexpr uint16_t TIMER_DIVIDER = 80; //  Hardware timer clock divider
  constexpr int64_t TIMER_ALARM = 30000; //Wenn 30ms kein Impuls kam, dann ging etwas schief
  //constexpr int64_t TIMER_ALARM = 4500000; //4,5sek - zum Testen
  constexpr esp_partition_type_t PARTITION_TYPE = ESP_PARTITION_TYPE_DATA;
  constexpr esp_partition_subtype_t PARTITION_SUBTYPE = (esp_partition_subtype_t)0xfe;
  constexpr uint32_t PARTITION_ID = 0x422455AA;
}

struct Pointers
{
  uint64_t *timerValueOnGpioIRQ;
  uint32_t *impulses;
  uint32_t *alarms;
  esp_mqtt_client_handle_t mqttClient;
  const char *mqttTopic;
  TaskHandle_t xTaskToNotify;
};

extern "C" void IRAM_ATTR gpio_isr_handler(void *arg)
{
  Pointers *p = (Pointers *)arg;
  assert(p);
  assert(p->timerValueOnGpioIRQ);
  assert(p->impulses);
  picoups_config::MAINS_TIMER_GROUP_RAW->hw_timer[picoups_config::MAINS_TIMER].update = 1;
  *(p->timerValueOnGpioIRQ) = ((uint64_t)picoups_config::MAINS_TIMER_GROUP_RAW->hw_timer[picoups_config::MAINS_TIMER].cnt_high << 32) | (picoups_config::MAINS_TIMER_GROUP_RAW->hw_timer[picoups_config::MAINS_TIMER].cnt_low);
  *(p->impulses)=*(p->impulses)+1;
  uint64_t load_val{0};
  picoups_config::MAINS_TIMER_GROUP_RAW->hw_timer[picoups_config::MAINS_TIMER].load_high = (uint32_t)(load_val >> 32);
  picoups_config::MAINS_TIMER_GROUP_RAW->hw_timer[picoups_config::MAINS_TIMER].load_low = (uint32_t)load_val;
  picoups_config::MAINS_TIMER_GROUP_RAW->hw_timer[picoups_config::MAINS_TIMER].reload = 1;
}

extern "C" void IRAM_ATTR mains_timer_isr(void *arg)
{
  Pointers *p = (Pointers *)arg;
  p->alarms++;
  timer_spinlock_take(picoups_config::MAINS_TIMER_GROUP);
  timer_group_clr_intr_status_in_isr(picoups_config::MAINS_TIMER_GROUP, picoups_config::MAINS_TIMER);
  timer_group_enable_alarm_in_isr(picoups_config::MAINS_TIMER_GROUP, picoups_config::MAINS_TIMER);
  configASSERT(p->xTaskToNotify != NULL);
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  vTaskNotifyGiveFromISR(p->xTaskToNotify, &xHigherPriorityTaskWoken);
  timer_spinlock_give(picoups_config::MAINS_TIMER_GROUP);
}

extern "C" void picoUPS(void *arg)
{
  Pointers *p = (Pointers *)arg;
  /* Select and initialize basic parameters of the timer */
  timer_config_t config = {};
  config.divider = picoups::TIMER_DIVIDER;
  config.counter_dir = TIMER_COUNT_UP;
  config.counter_en = TIMER_PAUSE;
  config.alarm_en = TIMER_ALARM_EN;
  config.auto_reload = TIMER_AUTORELOAD_EN;
  timer_init(picoups_config::MAINS_TIMER_GROUP, picoups_config::MAINS_TIMER, &config);

  timer_set_counter_value(picoups_config::MAINS_TIMER_GROUP, picoups_config::MAINS_TIMER, 0);
  timer_set_alarm_value(picoups_config::MAINS_TIMER_GROUP, picoups_config::MAINS_TIMER, picoups::TIMER_ALARM);
  timer_enable_intr(picoups_config::MAINS_TIMER_GROUP, picoups_config::MAINS_TIMER);
  timer_isr_register(picoups_config::MAINS_TIMER_GROUP, picoups_config::MAINS_TIMER, mains_timer_isr, arg, ESP_INTR_FLAG_IRAM, NULL);
  timer_start(picoups_config::MAINS_TIMER_GROUP, picoups_config::MAINS_TIMER);

  configASSERT(p->xTaskToNotify == NULL);
  p->xTaskToNotify = xTaskGetCurrentTaskHandle();
  char jsonBuffer[300];
  while (true)
  {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    if (*(p->impulses) > 1000/* && *(p->timerValueOnGpioIRQ) != 0 && 40 < (float)1000000 / (float)(*(p->timerValueOnGpioIRQ)))*/)
    {
      snprintf(jsonBuffer, sizeof(jsonBuffer), "{\"mainsloss\":%d}", *(p->impulses));
      ESP_LOGE(TAG, "%s", jsonBuffer);
      esp_mqtt_client_publish(p->mqttClient, p->mqttTopic, jsonBuffer, 0, 1, 0);
      int i=0;
      while (i<100)
      {
        ESP_LOGI(TAG, "Still Living...%d",i);
        vTaskDelay(2);
        i++;
      }
    }
  }
}

class PicoUPS
{
private:
  uint64_t timerValueOnGpioIRQ{0};
  uint32_t impulses{0};
  uint32_t alarms{0};
  uint32_t bootCounter{0};
  Pointers pointers{};


public:
  PicoUPS(esp_mqtt_client_handle_t mqttClient, const char *mqttTopic)
  {
    pointers.timerValueOnGpioIRQ = &(this->timerValueOnGpioIRQ);
    pointers.impulses = &(this->impulses);
    pointers.alarms = &(this->alarms);
    pointers.mqttClient=mqttClient;
    pointers.mqttTopic=mqttTopic;
    pointers.xTaskToNotify=NULL;
  }

  float GetFrequency()
  {
    if (timerValueOnGpioIRQ == 0)
      return 0;
    return (float)1000000 / (float)timerValueOnGpioIRQ;
  }

  uint32_t GetImpulses()
  {
    return impulses;
  }

  uint32_t GetAlarms()
  {
    return alarms;
  }

  uint32_t GetBootCounter()
  {
    return bootCounter;
  }

  void Init()
  {
    const esp_partition_t *partition = esp_partition_find_first(picoups::PARTITION_TYPE, picoups::PARTITION_SUBTYPE, NULL);
    if (!partition)
    {
      ESP_LOGE(TAG, "No Backup partition found. Fix partitions! Blocking application!");
      while (true)
        ;
    }
    uint32_t readBuf[4]{0};
    ESP_ERROR_CHECK(esp_partition_read(partition, 0, readBuf, 16));
    uint32_t partitionId = readBuf[0];

    if (partitionId == picoups::PARTITION_ID)
    {
      bootCounter = readBuf[3];
      ESP_LOGI(TAG, "Partition Boot Counter %d", bootCounter);
    }
    else
    {
      ESP_LOGI(TAG, "Partition Counter reset to 0");
      bootCounter = 0;
    }
    bootCounter++;

    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_POSEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << (int)picoups_config::PIN_MAINS);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(picoups_config::PIN_MAINS, gpio_isr_handler, &pointers);

    TaskHandle_t xHandle = NULL;
    xTaskCreate(picoUPS, "picpUPS", 4 * 4096, &pointers, 255, &xHandle);
    assert(xHandle);
  }
};
#undef TAG


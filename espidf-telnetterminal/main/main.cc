
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#include <wifi_sta.hh>


#include <esp_http_server.h>

#include "esp_system.h"


#include <string.h>
#include "nvs.h"
#include "nvs_flash.h"


#include "ESPTelnet.hh"
#include <megacli.hh>
#include <otacommand.hh>
#include <vector>


#define TAG "MAIN"


constexpr size_t COMMAND_LINE_MAX_LEN{80};



extern "C" void app_main(void)
{
  ESP_ERROR_CHECK(nvs_flash_init());
	ESP_ERROR_CHECK(WIFISTA::InitAndRun("smopla", "geheim"));

  httpd_config_t config = HTTPD_DEFAULT_CONFIG();

  httpd_handle_t server{nullptr};
  ESP_ERROR_CHECK(httpd_start(&server, &config));

  ESP_LOGI(TAG, "Init Telnet CLI");
  std::vector<CLI::AbstractCommand *> custom_commands = {new OTACommand(),};
	CLI::MegaCli *cli = new CLI::MegaCli(true, custom_commands);
	cli->InitAndRunCli();
  ESPTelnet telnet;
  assert(cli);
	telnet.initAndRunInTask(cli, 23);
  while(true){
    ESP_LOGI(TAG, "Free Heap %lu", esp_get_free_heap_size());
    vTaskDelay(pdMS_TO_TICKS(3000));
  }

  
}

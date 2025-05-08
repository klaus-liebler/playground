#include <stdio.h>
#include <esp_system.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <wifimanager.hh>
#include <syslogudp.hh>
#include <esp_http_server.h>
#define TAG "main"

uint8_t http_scatchpad[2048];

extern "C" void app_main(void)
{   
    ESP_ERROR_CHECK(nvs_flash_init());
    

    ESP_ERROR_CHECK(WIFIMGR::InitAndRun(false, http_scatchpad, sizeof(http_scatchpad)));
    httpd_handle_t server = nullptr;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    ESP_ERROR_CHECK(httpd_start(&server, &config));
    WIFIMGR::RegisterHTTPDHandlers(server);
    ESP_ERROR_CHECK(SyslogUdp::Setup("192.168.1.252", 514, LogProtocol::rfc5424, "PTENNCHEN01", Severity::Debug, false));
    int foo=1;
    while(true){
        ESP_LOGE(TAG, "Error %d", foo++);
        ESP_LOGW(TAG, "Warning %d", foo++);
        ESP_LOGI(TAG, "Info %d", foo++);
        SyslogUdp::Binlog(esp_timer_get_time()/1000, Severity::Informational, TAG, 0,0, "Syslog %d", foo++);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

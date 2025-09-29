#include <stdio.h>
#include <sys/param.h>
#include <unistd.h>
#include "esp_log.h"
#include "esp_netif.h"
#include <esp_flash_encrypt.h>
#include <esp_task_wdt.h>
#include <esp_sntp.h>
#include "esp_event.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "nvs_flash.h"
#include "lwip/ip_addr.h"
#include "sdkconfig.h"
#include <rgbled_rmt.hh>

#include "softap.hh"
#include "open62541.h"
#include "DHT22.h"
#include "model.h"

#define EXAMPLE_ESP_MAXIMUM_RETRY 10

#define TAG "OPCUA_ESP32"
#define SNTP_TAG "SNTP"
#define MEMORY_TAG "MEMORY"
#define ENABLE_MDNS 1

constexpr size_t LED_CNT{1};
UA_ServerConfig *config;
static UA_Boolean sntp_initialized = false;
static UA_Boolean running = true;
static UA_Boolean isServerCreated = false;
RTC_DATA_ATTR static int boot_count = 0;
static struct tm timeinfo;
static time_t now = 0;

static UA_StatusCode
UA_ServerConfig_setUriName(UA_ServerConfig *uaServerConfig, const char *uri, const char *name)
{
    // delete pre-initialized values
    UA_String_clear(&uaServerConfig->applicationDescription.applicationUri);
    UA_LocalizedText_clear(&uaServerConfig->applicationDescription.applicationName);

    uaServerConfig->applicationDescription.applicationUri = UA_String_fromChars(uri);
    uaServerConfig->applicationDescription.applicationName.locale = UA_STRING_NULL;
    uaServerConfig->applicationDescription.applicationName.text = UA_String_fromChars(name);

    for (size_t i = 0; i < uaServerConfig->endpointsSize; i++)
    {
        UA_String_clear(&uaServerConfig->endpoints[i].server.applicationUri);
        UA_LocalizedText_clear(
            &uaServerConfig->endpoints[i].server.applicationName);

        UA_String_copy(&uaServerConfig->applicationDescription.applicationUri,
                       &uaServerConfig->endpoints[i].server.applicationUri);

        UA_LocalizedText_copy(&uaServerConfig->applicationDescription.applicationName,
                              &uaServerConfig->endpoints[i].server.applicationName);
    }

    return UA_STATUSCODE_GOOD;
}
RGBLED::M<LED_CNT, RGBLED::Timing::WS2812> *leds;

UA_StatusCode
readRGBLED(UA_Server *server,
                const UA_NodeId *sessionId, void *sessionContext,
                const UA_NodeId *nodeId, void *nodeContext,
                UA_Boolean sourceTimeStamp, const UA_NumericRange *range,
                UA_DataValue *dataValue) {
    CRGB color;
    UA_Boolean state = leds->GetPixel(0, &color)== CRGB::Red;
    UA_Variant_setScalarCopy(&dataValue->value, &state,
                             &UA_TYPES[UA_TYPES_BOOLEAN]);
    dataValue->hasValue = true;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
writeRGBLED(UA_Server *server,
                  const UA_NodeId *sessionId, void *sessionContext,
                  const UA_NodeId *nodeId, void *nodeContext,
                 const UA_NumericRange *range, const UA_DataValue *data) {
    bool isRed=*(bool*)data->value.data;
    leds->SetPixel(0,isRed?CRGB::Red:CRGB::Black, true);
    return UA_STATUSCODE_GOOD;
}

void addRGBLED(UA_Server *server){
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "RGBLED");
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    bool initialValue=false;
    UA_Variant_setScalar(&attr.value, &initialValue, &UA_TYPES[UA_TYPES_BOOLEAN]);

    UA_NodeId currentNodeId = UA_NODEID_STRING(1, "Control RGBLED NodeId.");
    UA_QualifiedName currentName = UA_QUALIFIEDNAME(1, "Control RGBLED Name.");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_NodeId variableTypeNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE);

    UA_DataSource rgbledDs;
    rgbledDs.read = readRGBLED;
    rgbledDs.write = writeRGBLED;
    UA_Server_addDataSourceVariableNode(server, currentNodeId, parentNodeId,
                                        parentReferenceNodeId, currentName,
                                        variableTypeNodeId, attr,
                                        rgbledDs, NULL, NULL);
}


static void opcua_task(void *arg)
{
    // BufferSize's got to be decreased due to latest refactorings in open62541 v1.2rc.
    UA_Int32 sendBufferSize = 8192;
    UA_Int32 recvBufferSize = 8192;

    ESP_ERROR_CHECK(esp_task_wdt_add(NULL));

    ESP_LOGI(TAG, "Before UA_Server_new - Free Heap: %d bytes", xPortGetFreeHeapSize());
    UA_Server *server = UA_Server_new();
    ESP_LOGI(TAG, "After UA_Server_new - Free Heap: %d bytes", xPortGetFreeHeapSize());
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setMinimalCustomBuffer(config, 4840, 0, sendBufferSize, recvBufferSize);

    const char *appUri = "open62541.esp32.server";
    UA_String hostName = UA_STRING("192.168.4.1");

    UA_ServerConfig_setUriName(config, appUri, "OPC_UA_Server_ESP32");
     UA_String_clear(&config->customHostname);
    UA_String_copy(&hostName, &config->customHostname);
    ESP_LOGI(TAG, "Before Building Information Model - Free Heap : %d", xPortGetFreeHeapSize());
    /* Add Information Model Objects Here */
    addCurrentTemperatureDataSourceVariable(server);
    addRelay0ControlNode(server);
    addRelay1ControlNode(server);
    addRGBLED(server);

    ESP_LOGI(TAG, "Before UA_Server_run_startup - Free Heap : %d", xPortGetFreeHeapSize());
    UA_StatusCode retval = UA_Server_run_startup(server);
    if (retval != UA_STATUSCODE_GOOD)
    {
        ESP_LOGE(TAG, "Starting up the server failed with %s", UA_StatusCode_name(retval));
        return;
    }
    while (running)
    {
        UA_Server_run_iterate(server, false);
        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    UA_Server_run_shutdown(server);
   
    ESP_ERROR_CHECK(esp_task_wdt_delete(NULL));
}




extern "C" void app_main(void)
{
    ++boot_count;
    // Workaround for CVE-2019-15894
    nvs_flash_init();
    if (esp_flash_encryption_enabled())
    {
        esp_flash_write_protect_crypt_cnt();
    }

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }


    leds = new RGBLED::M<LED_CNT, RGBLED::Timing::WS2812>();
    ESP_ERROR_CHECK(leds->Init(RMT_CHANNEL_0, GPIO_NUM_48));
    
    leds->SetPixel(0, CRGB::Red, true);
    vTaskDelay(pdMS_TO_TICKS(200));
    leds->SetPixel(0, CRGB::Yellow, true);
    vTaskDelay(pdMS_TO_TICKS(200));
        leds->SetPixel(0, CRGB::Green, true);
    vTaskDelay(pdMS_TO_TICKS(200));
        leds->SetPixel(0, CRGB::Black, true);
    vTaskDelay(pdMS_TO_TICKS(200));

    
    SOFTAP::init(false, "test", "password");

    xTaskCreatePinnedToCore(opcua_task, "opcua_task", 24336, NULL, 10, NULL, 1);
    //xTaskCreatePinnedToCore(opcua_thread, "opcua_thread", 24336, NULL, 10, NULL, 1);
}

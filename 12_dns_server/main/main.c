#include <string.h>
#include <sys/socket.h>
#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "tcpip_adapter.h"
#include "nvs_flash.h"
#include "dns_server.h"
#include "web_server.h"

#define TAG  "dns_server"

esp_err_t event_handler(void *ctx, system_event_t *event)
{
   switch(event->event_id) {
    case SYSTEM_EVENT_AP_STACONNECTED:
        ESP_LOGI(TAG, "station:"MACSTR" join,AID=%d\n",
        MAC2STR(event->event_info.sta_connected.mac),
        event->event_info.sta_connected.aid);
        break;
    case SYSTEM_EVENT_AP_STADISCONNECTED:
        ESP_LOGI(TAG, "Wifi disconnected, try to connect ...");
        esp_wifi_connect();
        break;
    default:
        break;
    }
    return ESP_OK;
}

void initilalise_wifi(void)
{
    nvs_flash_init();
    tcpip_adapter_init();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_AP) );
    wifi_config_t ap_config = {
        .ap = {
            .ssid = "XUHONG-DNS",
            .password = "12345678",
            .ssid_len = 0,
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA_PSK
        }
    };
    ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_AP,&ap_config) );
    ESP_ERROR_CHECK( esp_wifi_start() );
    esp_wifi_connect();
}

void app_main(void)
{
    initilalise_wifi();
    my_udp_init();
    xTaskCreate(&web_server2, "web_server2", 2048*2, NULL, 5, NULL);
}

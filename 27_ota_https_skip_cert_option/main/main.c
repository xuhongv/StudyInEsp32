/*
 * @Author: your name
 * @Date: 2022-03-18 07:41:55
 * @LastEditTime: 2022-03-19 14:20:38
 * @LastEditors: Please set LastEditors
 * @Description: 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 * @FilePath: \esp-idf\examples\get-started\sample_project\main\main.c
 */
/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lwip/netdb.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_event_loop.h"
#include "esp_err.h"

#include "mbedtls/aes.h"
#include "mbedtls/sha256.h"
#include "mbedtls/compat-1.3.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "driver/uart.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/apps/sntp.h"
#include "lwip/apps/sntp_opts.h"
#include "mqtt_client.h"
#include "esp_tls.h"
#include "esp_ota_ops.h"
#include "nvs.h"
#include "cJSON.h"
#include "nvs_flash.h"
#include "esp_http_client.h"
#include "tcpip_adapter.h"
#include "https_ota.h"

#include "protocol_examples_common.h"

static const char *TAG = "MAIN_FILE:";

static void TaskOTAHttp(void *p)
{
    char URL[] = {"https://testaithinker.oss-cn-beijing.aliyuncs.com/esp32_ota.bin"};

    const otas_http_client_config config = {
        .cert_set = OTA_CERT_SSL_VERIFY_OPTIONAL,
        .skip_ssl_cert_set = false,
        .url_length = strlen(URL),
        .url = URL,
    };

    ESP_LOGI(TAG, "Free memory: %d bytes", esp_get_free_heap_size());

    if (start_https_ota(&config) == ESP_OK)
    {
        static int request_count;
        ESP_LOGI(TAG, "Completed %d requests and restart ", ++request_count);
        for (int countdown = 3; countdown >= 0; countdown--)
        {
            ESP_LOGI(TAG, "%d...", countdown);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
        esp_restart();
    }
    else
    {
        ESP_LOGE(TAG, "OTA Fail");
        ESP_LOGI(TAG, "Free memory: %d bytes", esp_get_free_heap_size());
    }

    vTaskDelete(NULL);
}

void app_main(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "date:%s,time:%s\n", __DATE__, __TIME__);

    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_ERROR_CHECK(example_connect());

    ESP_LOGI(TAG, "Free memory: %d bytes", esp_get_free_heap_size());

    ESP_LOGI(TAG, "Connected to AP, begin http...");

    xTaskCreate(&TaskOTAHttp, "TaskOTAHttp", 1024 * 10, NULL, 5, NULL);
}

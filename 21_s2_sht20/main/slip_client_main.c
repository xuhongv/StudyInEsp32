/*
 * @Author: your name
 * @Date: 2020-09-27 10:09:49
 * @LastEditTime: 2020-09-27 11:07:45
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \esp-idf\examples\me\SHT20_ESP32S2\DEMO\main\slip_client_main.c
 */
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_system.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "sht20.h"

static const char *TAG = "SHT20_EXAMPLE";

void app_main()
{
    while (true)
    {
        ESP_LOGI(TAG, "-----------------");
        ESP_LOGI(TAG, "T=%.2f℃", get_sht20_Temperature());
        ESP_LOGI(TAG, "H=%.2f%%", get_sht20_Humidity());
        vTaskDelay(2000 / portTICK_RATE_MS);
    }
}
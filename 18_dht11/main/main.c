
#include <string.h>
#include <openssl/ssl.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <esp_log.h>
#include <nvs_flash.h>

#include "dht11.h"

const static char *TAG = "demo_dht11";

/**
 * @description: 程序入口
 * @param {type} 
 * @return: 
 */
void app_main(void)
{

    ESP_ERROR_CHECK(nvs_flash_init());
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    int temp = 0;
    int hum = 0;

    //init
    dht11_init();

    while (1)
    {
        if (dht11_start_get(&temp, &hum))
        {
            ESP_LOGI(TAG, "[%lld] 温度 %i.%i℃ , 湿度 %i%%", esp_timer_get_time(), temp / 10, temp % 10, hum);
        }
        vTaskDelay(3000 / portTICK_PERIOD_MS);
    }
}
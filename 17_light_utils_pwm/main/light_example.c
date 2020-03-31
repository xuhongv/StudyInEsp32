#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "errno.h"

#include "nvs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/event_groups.h"

#include "light_driver.h"
#include "iot_led.h"

#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_log.h"

/**
 * @description:  HSV模型设置: 参考 http://www.yuangongju.com/color
 * @param {type} 
 * @return: 
 */
static void Task_set_hsv(void *parm)
{
    while (1)
    {
        light_driver_set_hsv(0, 100, 100); /**< red */
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        light_driver_set_hsv(240, 100, 100); /**< blue */
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        light_driver_set_hsv(120, 100, 100); /**< green */
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        light_driver_set_hsv(270, 98, 99); /**< color #8306fc */
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        light_driver_set_hsv(60, 100, 100); /**<yellow*/
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

/**
 * @description:  RGB光设置
 * @param {type} 
 * @return: 
 */
static void Task_set_rgb(void *parm)
{
    while (1)
    {
        light_driver_set_rgb(255, 0, 0); /**< red */
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        light_driver_set_rgb(255, 128, 0); /**< orange */
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        light_driver_set_rgb(255, 255, 0); /**< yellow */
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        light_driver_set_rgb(0, 255, 0); /**< green */
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        light_driver_set_rgb(255, 0, 0); /**< green */
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        light_driver_set_rgb(0, 0, 255); /**< green */
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        light_driver_set_rgb(128, 255, 0); /**< green */
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        light_driver_set_rgb(128, 128, 0); /**< green */
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

/**
 * @description:  RGB呼吸光设置
 * @param {type} 
 * @return: 
 */
static void Task_set_rgb_with_breath(void *parm)
{

    while (1)
    {
        light_driver_breath_start(255, 0, 0); /**< red */
        vTaskDelay(1500 / portTICK_PERIOD_MS);
        light_driver_breath_start(255, 128, 0); /**< orange */
        vTaskDelay(2500 / portTICK_PERIOD_MS);
        light_driver_breath_start(255, 255, 0); /**< yellow */
        vTaskDelay(2500 / portTICK_PERIOD_MS);
        light_driver_breath_start(0, 255, 0); /**< green */
        vTaskDelay(1500 / portTICK_PERIOD_MS);
    }

    vTaskDelete(NULL);
}

/**
 * @description:  冷暖光设置
 * @param {type} 
 * @return: 
 */
static void Task_brightness_temperature(void *parm)
{
    light_driver_set_brightness(100);
    vTaskDelay(1500 / portTICK_PERIOD_MS);
    light_driver_set_color_temperature(100);
    vTaskDelay(1500 / portTICK_PERIOD_MS);
    light_driver_set_brightness(10);
    vTaskDelay(1500 / portTICK_PERIOD_MS);
    light_driver_set_color_temperature(50);
    vTaskDelay(1500 / portTICK_PERIOD_MS);

    xTaskCreate(Task_set_hsv, "Task_set_hsv", 1024 * 2, NULL, 8, NULL); // 创建任务

    vTaskDelete(NULL);
}

/**
 * @description: 程序入口
 * @param {type} 
 * @return: 
 */
void app_main(void)
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    printf("\n\n-------------------------------- Get Systrm Info Start------------------------------------------\n");
    //获取IDF版本
    printf("     SDK version:%s\n", esp_get_idf_version());
    //获取芯片可用内存
    printf("     esp_get_free_heap_size : %d  \n", esp_get_free_heap_size());
    //获取从未使用过的最小内存
    printf("     esp_get_minimum_free_heap_size : %d  \n", esp_get_minimum_free_heap_size());
    //获取mac地址（station模式）
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    printf("esp_read_mac(): %02x:%02x:%02x:%02x:%02x:%02x \n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    printf("\n\n-------------------------------- Get Systrm Info End------------------------------------------\n");

    /**
     * NOTE:
     *  If the module has SPI flash, GPIOs 6-11 are connected to the module’s integrated SPI flash and PSRAM.
     *  If the module has PSRAM, GPIOs 16 and 17 are connected to the module’s integrated PSRAM.
     *  and gpio_num <34
     */
    light_driver_config_t driver_config = {
        .gpio_red = CONFIG_LIGHT_GPIO_RED,
        .gpio_green = CONFIG_LIGHT_GPIO_GREEN,
        .gpio_blue = CONFIG_LIGHT_GPIO_BLUE,
        .gpio_cold = CONFIG_LIGHT_GPIO_COLD,
        .gpio_warm = CONFIG_LIGHT_GPIO_WARM,
        .fade_period_ms = CONFIG_LIGHT_FADE_PERIOD_MS,
        .blink_period_ms = CONFIG_LIGHT_BLINK_PERIOD_MS,
    };

    ESP_ERROR_CHECK(light_driver_init(&driver_config));

    xTaskCreate(Task_brightness_temperature, "Task_brightness_temperature", 1024 * 2, NULL, 8, NULL); // 创建任务
}

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "errno.h"
#include "iot_led.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <esp_log.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/event_groups.h"

#include "light_driver.h"


static const char *TAG = "light_example";


void app_main()
{

    /**
     * NOTE:
     *  If the module has SPI flash, GPIOs 6-11 are connected to the module’s integrated SPI flash and PSRAM.
     *  If the module has PSRAM, GPIOs 16 and 17 are connected to the module’s integrated PSRAM.
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

    /**
     * @brief Light driver initialization
     */
    (light_driver_init(&driver_config));
    light_driver_set_switch(true);

    ESP_LOGE(TAG,"hello world");
}

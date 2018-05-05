/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

//宏定义
#define BLINK_GPIO 16

void blink_task(void *pvParameter)
{
    //短脚设置
    gpio_pad_select_gpio(BLINK_GPIO);
    //设置为输出
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
    while(1) {
        //设置低电平
        gpio_set_level(BLINK_GPIO, 0);
		//延时1s
        vTaskDelay(1000 / portTICK_PERIOD_MS);
          //设置高电平
        gpio_set_level(BLINK_GPIO, 1);
		//延时1s
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void app_main()
{
    xTaskCreate(&blink_task, "blink_task", configMINIMAL_STACK_SIZE, NULL, 5, NULL);
}

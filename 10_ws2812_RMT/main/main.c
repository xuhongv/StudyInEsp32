

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <soc/rmt_struct.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <driver/gpio.h>
#include <stdio.h>
#include "ws2812.h"

#define WS2812_PIN 22
#define WS2812_COUNTS 20 // 灯珠个数
#define delay_ms(ms) vTaskDelay((ms) / portTICK_RATE_MS)

/**
 * @description: 彩虹渐变效果演示
 * @param {type} 
 * @return: 
 */
void TaskRainbow(void *pvParameters)
{
  const uint8_t anim_step = 10;
  const uint8_t anim_max = 250;

  const uint8_t delay = 25; // 2种颜色之间的渐变时间间隔

  rgbValue color = getRGBValue(anim_max, 0, 0);
  uint8_t step = 0;
  rgbValue color2 = getRGBValue(anim_max, 0, 0);
  uint8_t step2 = 0;
  rgbValue *pixels;

  pixels = malloc(sizeof(rgbValue) * WS2812_COUNTS);

  while (1)
  {
    color = color2;
    step = step2;

    for (uint8_t i = 0; i < WS2812_COUNTS; i++)
    {
      pixels[i] = color;

      if (i == 1)
      {
        color2 = color;
        step2 = step;
      }

      switch (step)
      {
      case 0:
        color.g += anim_step;
        if (color.g >= anim_max)
          step++;
        break;
      case 1:
        color.r -= anim_step;
        if (color.r == 0)
          step++;
        break;
      case 2:
        color.b += anim_step;
        if (color.b >= anim_max)
          step++;
        break;
      case 3:
        color.g -= anim_step;
        if (color.g == 0)
          step++;
        break;
      case 4:
        color.r += anim_step;
        if (color.r >= anim_max)
          step++;
        break;
      case 5:
        color.b -= anim_step;
        if (color.b == 0)
          step = 0;
        break;
      }
    }

    ws2812_setColors(pixels);

    delay_ms(delay);
  }
}

void app_main()
{
  nvs_flash_init();

  //初始化
  ws2812_init(WS2812_PIN, WS2812_COUNTS);
  //彩虹渐变效果演示
  //xTaskCreate(TaskRainbow, "TaskRainbow Demo", 4096, NULL, 10, NULL);


  /*  下面演示如果看到实际效果应该是  红色---绿色---蓝色
   *   但是如果看到的是   绿色---红色---蓝色 请替换调用 ws2812_setColor_grb()方法即可！
   *     原因在于：不同的灯珠生产商家定义不一样
   */
  while (1)
  {
    ws2812_setColor(254,0,0);//红色
    delay_ms(1000);
    ws2812_setColor(0,254,0);//绿色
    delay_ms(1000);
    ws2812_setColor(0,0,254);//蓝色
    delay_ms(1000);
  }
}

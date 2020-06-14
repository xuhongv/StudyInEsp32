#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/rmt.h"
#include "led_strip.h"

static const char *TAG = "WS2812B_EXAMPLE";

#define RMT_TX_CHANNEL RMT_CHANNEL_0
#define EXAMPLE_CHASE_SPEED_MS (500)
uint32_t red = 0;
uint32_t green = 0;
uint32_t blue = 0;
uint16_t hue = 0;
uint16_t start_rgb = 0;

static led_strip_t *ws2812_ret = NULL;

/**
 * @description: 彩虹效果
 * @param {type} 
 * @return: 
 */
void TaskWS2812Ranbow(void *p)
{

  while (true)
  {
    for (int i = 0; i < 3; i++)
    {
      for (int j = i; j < 24; j += 3)
      {
        // Build RGB values
        hue = j * 360 / 24 + start_rgb;
        led_strip_hsv2rgb(hue, 100, 100, &red, &green, &blue);
        // Write RGB values to strip driver
        ESP_ERROR_CHECK(ws2812_ret->set_pixel(ws2812_ret, j, red, green, blue));
      }

      // Flush RGB values to LEDs
      ESP_ERROR_CHECK(ws2812_ret->refresh(ws2812_ret, 100));
      vTaskDelay(pdMS_TO_TICKS(10));
      ws2812_ret->clear(ws2812_ret, 50);
      vTaskDelay(pdMS_TO_TICKS(10));
    }
    start_rgb += 60;
  }
}

/**
 * @description: 一个一个逐渐亮起
 * @param {type} 
 * @return: 
 */
void TaskWS2812OneByOne(void *p)
{
  uint8_t nums = 5;

  while (nums--)
  {
    for (int j = 0; j < WS2812B_RMT_LED_NUMBER; j++)
    {
      // Build RGB values
      hue = j * 360 / WS2812B_RMT_LED_NUMBER + start_rgb;
      led_strip_hsv2rgb(hue, 100, 100, &red, &green, &blue);
      // Write RGB values to strip driver
      ESP_ERROR_CHECK(ws2812_ret->set_pixel(ws2812_ret, j, red, green, blue));
      ESP_ERROR_CHECK(ws2812_ret->refresh(ws2812_ret, 100));
      vTaskDelay(pdMS_TO_TICKS(500));
    }

    ws2812_ret->clear(ws2812_ret, 50);
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
  xTaskCreate(TaskWS2812Ranbow, "TaskWS2812Ranbow", 1024 * 2, NULL, 8, NULL);
  vTaskDelete(NULL);
}

/**
 * @description:  程序入口
 * @param {type} 
 * @return: 
 */
void app_main(void)
{

  rmt_config_t config = RMT_DEFAULT_CONFIG_TX(WS2812B_RMT_LED_GPIO, RMT_TX_CHANNEL);
  // set counter clock to 40MHz
  config.clk_div = 2;

  ESP_ERROR_CHECK(rmt_config(&config));
  ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, 0));

  // install ws2812 driver
  led_strip_config_t strip_config = LED_STRIP_DEFAULT_CONFIG(WS2812B_RMT_LED_NUMBER, (led_strip_dev_t)config.channel);

  ws2812_ret = led_strip_new_rmt_ws2812(&strip_config);

  if (!ws2812_ret)
  {
    ESP_LOGE(TAG, "install WS2812 driver failed");
  }

  // 关闭所有
  ESP_ERROR_CHECK(ws2812_ret->clear(ws2812_ret, 100));
  //三色循环
  uint8_t nums = 2;
  while (nums--)
  {
    ws2812_ret->set_rgb(ws2812_ret, 100, 255, 0, 0);
    vTaskDelay((1000 / portTICK_RATE_MS));
    ws2812_ret->set_rgb(ws2812_ret, 100, 0, 255, 0);
    vTaskDelay((1000 / portTICK_RATE_MS));
    ws2812_ret->set_rgb(ws2812_ret, 100, 0, 0, 255);
    vTaskDelay((1000 / portTICK_RATE_MS));
  }
  // 关闭所有
  ESP_ERROR_CHECK(ws2812_ret->clear(ws2812_ret, 100));

  xTaskCreate(TaskWS2812OneByOne, "TaskWS2812OneByOne", 1024 * 3, NULL, 8, NULL);
}

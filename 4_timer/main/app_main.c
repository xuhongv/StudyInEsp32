#include <stdio.h>
#include "esp_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "soc/timer_group_struct.h"
#include "driver/periph_ctrl.h"
#include "driver/timer.h"
#include "esp_timer.h"

//静态声明2个定时器的回调函数
void test_timer_periodic_cb(void *arg);
void test_timer_once_cb(void *arg);

//定义2个定时器句柄
esp_timer_handle_t test_p_handle = 0;
esp_timer_handle_t test_o_handle = 0;

//定义一个单次运行的定时器结构体
esp_timer_create_args_t test_once_arg = { .callback = &test_timer_once_cb, //设置回调函数
		.arg = NULL, //不携带参数
		.name = "TestOnceTimer" //定时器名字
		};
//定义一个周期重复运行的定时器结构体
esp_timer_create_args_t test_periodic_arg = { .callback =
		&test_timer_periodic_cb, //设置回调函数
		.arg = NULL, //不携带参数
		.name = "TestPeriodicTimer" //定时器名字

		};

void test_timer_periodic_cb(void *arg) {

	int64_t tick = esp_timer_get_time();

	printf("方法回调名字: %s , 距离定时器开启时间间隔 = %lld \r\n", __func__, tick);

	if (tick > 100000000) {
		//停止定时器工作，并获取是否停止成功
		esp_err_t err = esp_timer_stop(test_p_handle);
		printf("要停止的定时器名字：%s , 是否停止成功：%s", test_periodic_arg.name,
				err == ESP_OK ? "ok!\r\n" : "failed!\r\n");
		err = esp_timer_delete(test_p_handle);
		printf("要删除的定时器名字：%s , 是否停止成功：%s", test_periodic_arg.name,
				err == ESP_OK ? "ok!\r\n" : "failed!\r\n");
	}

	//低电平
	gpio_set_level(16, 0);
	//延迟
	vTaskDelay(1000 / portTICK_PERIOD_MS);
	//高电平
	gpio_set_level(16, 1);
	//延迟
	vTaskDelay(1000 / portTICK_PERIOD_MS);

}

void test_timer_once_cb(void *arg) {

	int64_t tick = esp_timer_get_time();

	printf("方法回调名字: %s , 距离定时器开启时间间隔 = %lld \r\n", __func__, tick);

	esp_err_t err = esp_timer_delete(test_o_handle);

	printf("要删除的定时器名字：%s , 是否停止成功：%s", test_periodic_arg.name,
			err == ESP_OK ? "ok!\r\n" : "failed!\r\n");

}

void app_main() {

	gpio_pad_select_gpio(16);
	gpio_set_direction(16, GPIO_MODE_OUTPUT);

	//开始创建一个重复周期的定时器并且执行
	esp_err_t err = esp_timer_create(&test_periodic_arg, &test_p_handle);
	err = esp_timer_start_periodic(test_p_handle, 1000 * 1000);
	printf("重复周期运行的定时器创建状态码: %s", err == ESP_OK ? "ok!\r\n" : "failed!\r\n");

	//开始创建一个单次周期的定时器并且执行
	err = esp_timer_create(&test_once_arg, &test_o_handle);
	err = esp_timer_start_once(test_o_handle, 10 * 1000 * 1000);
	printf("单次运行的定时器创建状态码: %s", err == ESP_OK ? "ok!\r\n" : "failed!\r\n");

}


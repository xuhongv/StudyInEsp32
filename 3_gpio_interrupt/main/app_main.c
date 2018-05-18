#include <stdio.h>
#include "esp_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"

//宏定义一个GPIO口用于GPIO口的输入输出
#define BLINK_GPIO 16

//设置GPIO高低电平输出
void fun_set_gpio_level() {

	//第一种方式配置
	//gpio_pad_select_gpio(BLINK_GPIO);
	//gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);

	//第二种方式配置
	gpio_config_t io_conf;
	//进制中断
	io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
	//选择为输出模式
	io_conf.mode = GPIO_MODE_OUTPUT;
	//配置GPIO_OUT寄存器
	io_conf.pin_bit_mask = GPIO_SEL_16;
	//禁止下拉
	io_conf.pull_down_en = 0;
	//禁止上拉
	io_conf.pull_up_en = 0;
	//最后配置使能
	gpio_config(&io_conf);

	//输出低电平
	gpio_set_level(BLINK_GPIO, 0);

}

//设置获取指定的GPIO的输入电平
void fun_get_gpio_level() {

	//第一种方式配置
	//gpio_pad_select_gpio(BLINK_GPIO);
	//gpio_set_direction(BLINK_GPIO, GPIO_MODE_INPUT);

	//第二种方式配置
	gpio_config_t io_conf;
	//进制中断
	io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
	//选择为输出模式
	io_conf.mode = GPIO_MODE_INPUT;
	//配置GPIO_OUT寄存器
	io_conf.pin_bit_mask = GPIO_SEL_16;
	//禁止下拉
	io_conf.pull_down_en = 0;
	//禁止上拉
	io_conf.pull_up_en = 0;
	//最后配置使能
	gpio_config(&io_conf);

	/* 挂起500ms. */
	const portTickType xDelay = 500 / portTICK_RATE_MS;

	while (1) {
		printf(" Current Gpio16 Level is : %d \r\n\r\n",
				gpio_get_level(BLINK_GPIO));
		vTaskDelay(xDelay);
	}

}

/***************************************************************************************/
/*************            以下是中断函数的使用                                        *****************************/
/***************************************************************************************/

#define GPIO_INPUT_IO_0     4

static xQueueHandle gpio_evt_queue = NULL; //定义一个队列返回变量

void IRAM_ATTR gpio_isr_handler(void* arg) {
	//把中断消息插入到队列的后面，将gpio的io参数传递到队列中
	uint32_t gpio_num = (uint32_t) arg;
	xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

//低电平触发的回调方法
void gpio_low_interrupt_callBack(void* arg) {
	printf(" \r\n into gpio_low_interrupt_callBack ...\r\n  ");
	uint32_t io_num;
	while (1) {
		//不断读取gpio队列，读取完后将删除队列
		if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
			printf("GPIO[%d] 中断触发, 当前的电压: %d\n", io_num,
					gpio_get_level(io_num));
		}
	}
}

void fun_set_gpio_low_interrupt() {

	//GPIO口结构体定义
	gpio_config_t io_conf;
	//下降沿触发
	io_conf.intr_type = GPIO_INTR_NEGEDGE;
	//选择为输出模式
	io_conf.mode = GPIO_MODE_INPUT;
	//配置GPIO_OUT寄存器
	io_conf.pin_bit_mask = GPIO_SEL_4;
	//设置下拉
	io_conf.pull_down_en = 0;
	//设置上拉
	io_conf.pull_up_en = 1;
	//最后配置使能
	gpio_config(&io_conf);

	//注册中断服务
	gpio_install_isr_service(1);
	//设置GPIO的中断回调函数
	gpio_isr_handler_add(GPIO_INPUT_IO_0, gpio_isr_handler,
			(void*) GPIO_INPUT_IO_0);

	//创建一个消息队列，从中获取队列句柄
	gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));

	//新建队列的
	xTaskCreate(gpio_low_interrupt_callBack //任务函数
			, "gpio_task_example" //任务名字
			, 2048  //任务堆栈大小
			, NULL  //传递给任务函数的参数
			, 10   //任务优先级
			, NULL); //任務句柄

}

void app_main() {

	//设置gpio的电平输出
	//fun_set_gpio_level();

	//获取gpio的电平输入
	//fun_get_gpio_level();

	//gpio中断
	fun_set_gpio_low_interrupt();
}


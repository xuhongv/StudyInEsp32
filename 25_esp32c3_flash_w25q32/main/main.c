#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "my_spi_flash.h"

#define TAG "AiThinker-W25QXX::"

const uint8_t TEXT_Buffer[] = {"I am aithinker xuhongv"};
#define SIZE sizeof(TEXT_Buffer)

void app_main()
{

	SPI_FLASH_Init();

	uint8_t datatemp[256] = {0};

	uint16_t FLASH_SIZE = 8 * 1024 * 1024; //FLASH 大小为8M字节

	ESP_LOGI(TAG, "Write mySaveBuff length:%d", SIZE);
	ESP_LOGI(TAG, "Write mySaveBuff:%s\n", TEXT_Buffer);

	W25QXX_Write(TEXT_Buffer, FLASH_SIZE - 100, SIZE);
	W25QXX_Read(datatemp, FLASH_SIZE - 100, SIZE);

	ESP_LOGI(TAG, "Get mySaveBuff:%s \n", datatemp);
}

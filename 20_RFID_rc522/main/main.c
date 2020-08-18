
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "rc522.h"
#include "driver/gpio.h"

static const char *TAG = "main";

void tag_handler(uint8_t *serial_no)
{
    for (int i = 0; i < 5; i++)
    {
        printf("%#x ", serial_no[i]);
    }

    gpio_set_level(12, 0);
    vTaskDelay(300 / portTICK_PERIOD_MS);
    gpio_set_level(12, 1);
    printf("\n");
}

void app_main(void)
{

    gpio_pad_select_gpio(12);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(12, GPIO_MODE_OUTPUT);

    const rc522_start_args_t start_args = {
        .miso_io = 25,
        .mosi_io = 23,
        .sck_io = 19,
        .sda_io = 22,
        .callback = &tag_handler
    };

    rc522_start(start_args);
}

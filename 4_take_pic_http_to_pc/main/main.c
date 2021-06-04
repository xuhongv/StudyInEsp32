#include <esp_event_loop.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include <string.h>
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "app_wifi.h"
#include "esp_camera.h"

#include "cJSON.h"
#include "esp_http_client.h"

#define MAX_HTTP_RECV_BUFFER 512

#define CAM_PIN_PWDN 32
#define CAM_PIN_RESET -1
#define CAM_PIN_XCLK 0
#define CAM_PIN_SIOD 26
#define CAM_PIN_SIOC 27
#define CAM_PIN_D7 35
#define CAM_PIN_D6 34
#define CAM_PIN_D5 39
#define CAM_PIN_D4 36
#define CAM_PIN_D3 21
#define CAM_PIN_D2 19
#define CAM_PIN_D1 18
#define CAM_PIN_D0 5
#define CAM_PIN_VSYNC 25
#define CAM_PIN_HREF 23
#define CAM_PIN_PCLK 22

static const char *TAG = "TAG:";
#define ECHO_TEST_TXD (GPIO_NUM_1)
#define ECHO_TEST_RXD (GPIO_NUM_3)
#define ECHO_LOG_TXD (GPIO_NUM_2)
#define ECHO_LOG_RXD (GPIO_NUM_4)
#define ECHO_TEST_RTS (UART_PIN_NO_CHANGE)
#define ECHO_TEST_CTS (UART_PIN_NO_CHANGE)
#define BUF_SIZE (2048)

static xQueueHandle ParseJSONQueueHandler = NULL;                   //解析json数据的队列
static xTaskHandle mHandlerParseJSON = NULL, handleTaskUart = NULL; //任务队列

typedef struct __User_data
{
    char allData[1024];
    int dataLen;
} User_data;

User_data user_data;

static camera_config_t camera_config = {

    .pin_pwdn = CAM_PIN_PWDN,
    .pin_reset = CAM_PIN_RESET,
    .pin_xclk = CAM_PIN_XCLK,
    .pin_sscb_sda = CAM_PIN_SIOD,
    .pin_sscb_scl = CAM_PIN_SIOC,

    .pin_d7 = CAM_PIN_D7,
    .pin_d6 = CAM_PIN_D6,
    .pin_d5 = CAM_PIN_D5,
    .pin_d4 = CAM_PIN_D4,
    .pin_d3 = CAM_PIN_D3,
    .pin_d2 = CAM_PIN_D2,
    .pin_d1 = CAM_PIN_D1,
    .pin_d0 = CAM_PIN_D0,
    .pin_vsync = CAM_PIN_VSYNC,
    .pin_href = CAM_PIN_HREF,
    .pin_pclk = CAM_PIN_PCLK,

    //XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
    .xclk_freq_hz = 20000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_JPEG, //YUV422,GRAYSCALE,RGB565,JPEG
    .frame_size = FRAMESIZE_VGA,    //QQVGA-UXGA Do not use sizes above QVGA when not JPEG

    .jpeg_quality = 10, //0-63 lower number means higher quality
    .fb_count = 1       //if more than one, i2s runs in continuous mode. Use only with JPEG
};

/* 
 * @Description: 解析下发数据的队列逻辑处理
 * @param: null
 * @return: 
*/
void Task_ParseJSON(void *pvParameters)
{
    printf("[SY] Task_ParseJSON_Message creat ... \n");

    char data[] = {0xB0, 0, 0};
    size_t _jpg_buf_len;
    uint8_t *_jpg_buf;
    camera_fb_t *pic = NULL;
    esp_err_t res = ESP_OK;
    size_t fb_len = 0;
    while (1)
    {
        struct __User_data *pMqttMsg;

        printf("Task_ParseJSON_Message xQueueReceive wait [%d] ... \n", esp_get_free_heap_size());
        xQueueReceive(ParseJSONQueueHandler, &pMqttMsg, portMAX_DELAY);

        printf("Task_ParseJSON_Message xQueueReceive get [%s] ... \n", pMqttMsg->allData);

        ////首先整体判断是否为一个json格式的数据
        cJSON *pJsonRoot = cJSON_Parse(pMqttMsg->allData);
        //如果是否json格式数据
        if (pJsonRoot == NULL)
        {
            printf("[SY] Task_ParseJSON_Message xQueueReceive not json ... \n");
            data[1] = 0xA0;
            goto __cJSON_Delete;
        }

        cJSON *pJSON_Item_cmd = cJSON_GetObjectItem(pJsonRoot, "cmd");

        if (strcmp(pJSON_Item_cmd->valuestring, "Reboot") == 0)
        {
            data[2] = data[1] + 0xB0;
            uart_write_bytes(UART_NUM_1, (const char *)data, 3);
            ESP_LOGE(TAG, "reboot after 2s ...");
            //延迟2秒后重启设备
            vTaskDelay(2000 / portTICK_PERIOD_MS);
            esp_restart();
        }
        else if (strcmp(pJSON_Item_cmd->valuestring, "Get Picture") == 0)
        {
            while (1)
            {
                //开始拍照
                pic = esp_camera_fb_get();

                if (!pic)
                {
                    ESP_LOGE(TAG, "Camera capture failed");
                }
                else
                {
                    //拍照成功，获取其大小、尺寸等信息
                    ESP_LOGI(TAG, "Camera capture OK , Its size was: %zu bytes", pic->len);
                    ESP_LOGI(TAG, "Camera capture OK , Its width was: %d", pic->width);
                    ESP_LOGI(TAG, "Camera capture OK , Its height was: %d ", pic->height);

                    //把图片放在body里面
                    uart_write_bytes(UART_NUM_1, (const char *)pic->buf, pic->len);
                    //vTaskDelay(200 / portTICK_RATE_MS);
                }

                esp_camera_fb_return(pic);
            }
        }

        //
        // Write data back to the UART
        // uart_write_bytes(uart_num, (const char *)data, len);

        //set_rgb(pJSON_Item_Red->valueint, pJSON_Item_Green->valueint, pJSON_Item_Blue->valueint);

    __cJSON_Delete:

        cJSON_Delete(pJsonRoot);
    }
}

static void http_test_task(void *pvParameters)
{

    //     //等待Wi-Fi连接成功
    //     app_wifi_wait_connected();

    //     ESP_LOGI(TAG, "Http Start");

    //     //配置服务器相关信息
    //     esp_http_client_config_t config = {
    //         .url = "http://www.your-name.com/index.php",
    //         .method = HTTP_METHOD_POST,
    //         .event_handler = _http_event_handler,
    //         .buffer_size = 4096,
    //         .timeout_ms = 10000,
    //     };

    //     esp_http_client_handle_t client = esp_http_client_init(&config);

    // end:
    // {
    //
    // }

    ESP_LOGI(TAG, "Http End");
    vTaskDelete(NULL);
}

static void TaskUart()
{

    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE};
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, ECHO_LOG_TXD, ECHO_LOG_RXD, ECHO_TEST_RTS, ECHO_TEST_CTS);
    uart_driver_install(UART_NUM_1, BUF_SIZE * 2, 0, 0, NULL, 0);

    uart_param_config(UART_NUM_0, &uart_config);
    uart_set_pin(UART_NUM_0, ECHO_TEST_TXD, ECHO_TEST_RXD, ECHO_TEST_RTS, ECHO_TEST_CTS);
    uart_driver_install(UART_NUM_0, BUF_SIZE, 0, 0, NULL, 0);

    uint8_t *data = (uint8_t *)malloc(BUF_SIZE);

    while (1)
    {
        // Read data from the UART
        int len = uart_read_bytes(UART_NUM_1, data, BUF_SIZE, 20 / portTICK_RATE_MS);
        if (len > 0)
        {
            if (data[0] == 0xF0)
            {
                switch (data[1])
                {
                case 0xA0:
                {
                    //发送数据到队列
                    struct __User_data *pTmper;
                    sprintf(user_data.allData, "{\"cmd\":\"Reboot\"}");
                    pTmper = &user_data;
                    user_data.dataLen = strlen(user_data.allData);
                    xQueueSend(ParseJSONQueueHandler, (void *)&pTmper, portMAX_DELAY);
                }
                break;
                case 0xA1:
                {
                    //发送数据到队列
                    struct __User_data *pTmper;
                    sprintf(user_data.allData, "{\"cmd\":\"Get Picture\"}");
                    pTmper = &user_data;
                    user_data.dataLen = strlen(user_data.allData);
                    xQueueSend(ParseJSONQueueHandler, (void *)&pTmper, portMAX_DELAY);
                }
                break;

                default:
                    break;
                }
            }
        }
    }
}

void app_main()
{

    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);

    //initialize the camera
    ret = esp_camera_init(&camera_config);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Camera Init Failed");
    }

    //关闭LED
    gpio_config_t io_conf;
    //disable interrupt
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = (1ULL << GPIO_NUM_4);
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);
    gpio_set_level(GPIO_NUM_4, 1);

    if (handleTaskUart == NULL)
        ret = xTaskCreate(TaskUart, "TaskUart", 1024, NULL, 10, NULL);

    if (ParseJSONQueueHandler == NULL)
        ParseJSONQueueHandler = xQueueCreate(5, sizeof(struct __User_data *));

    //开启json解析线程
    if (mHandlerParseJSON == NULL)
    {
        xTaskCreate(Task_ParseJSON, "Task_ParseJSON", 1024 * 3, NULL, 4, &mHandlerParseJSON);
    }

    if (ret != pdPASS)
    {
        printf("create TaskXMqttRecieve thread failed.\n");
    }

    //xTaskCreate(&http_test_task, "http_test_task", 8192, NULL, 5, NULL);
}
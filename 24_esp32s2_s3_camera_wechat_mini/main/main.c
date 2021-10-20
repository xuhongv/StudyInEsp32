#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_camera.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"
#include "esp_http_client.h"
#include "cJSON.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "mqtt_client.h"

#define PWDN_GPIO_NUM 43
#define RESET_GPIO_NUM 44

#define VSYNC_GPIO_NUM 6
#define HREF_GPIO_NUM 7
#define PCLK_GPIO_NUM 13
#define XCLK_GPIO_NUM 15

#define SIOD_GPIO_NUM 4
#define SIOC_GPIO_NUM 5

#define Y9_GPIO_NUM 16
#define Y8_GPIO_NUM 17
#define Y7_GPIO_NUM 18
#define Y6_GPIO_NUM 12
#define Y5_GPIO_NUM 11
#define Y4_GPIO_NUM 10
#define Y3_GPIO_NUM 9
#define Y2_GPIO_NUM 8

static const char *TAG = "example:take_picture_s3";
esp_mqtt_client_handle_t client;
char deviceUUID[17];

struct Pic_param_t
{
    char pic_name_p[50];
    char app_sub_topic[50];
};

typedef struct Pic_param_t *pic_param_t;

static camera_config_t camera_config = {

    .pin_pwdn = GPIO_NUM_35,
    .pin_reset = GPIO_NUM_36,
    .pin_xclk = GPIO_NUM_15,
    .pin_sscb_sda = GPIO_NUM_4,
    .pin_sscb_scl = GPIO_NUM_5,

    .pin_d7 = GPIO_NUM_16,
    .pin_d6 = GPIO_NUM_17,
    .pin_d5 = GPIO_NUM_18,
    .pin_d4 = GPIO_NUM_12,
    .pin_d3 = GPIO_NUM_11,
    .pin_d2 = GPIO_NUM_10,
    .pin_d1 = GPIO_NUM_9,
    .pin_d0 = GPIO_NUM_8,
    .pin_vsync = GPIO_NUM_6,
    .pin_href = GPIO_NUM_7,
    .pin_pclk = GPIO_NUM_13,

    //XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
    .xclk_freq_hz = 20000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_JPEG, //YUV422,GRAYSCALE,RGB565,JPEG
    .frame_size = FRAMESIZE_XGA,    //QQVGA-UXGA Do not use sizes above QVGA when not JPEG

    .jpeg_quality = 10, //0-63 lower number means higher quality
    .fb_count = 1       //if more than one, i2s runs in continuous mode. Use only with JPEG

};

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id)
    {
    case HTTP_EVENT_ERROR:
        ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGE(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        ESP_LOGE(TAG, "response->buffer: %s", (char *)(evt->data));

        esp_mqtt_client_publish(client, "/light/deviceOut", (char *)(evt->data), evt->data_len, 1, 0);

        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
        break;
    }
    return ESP_OK;
}

static void Task_upload_pic(void *pvParameters)
{

    ESP_LOGI(TAG, "Http Start");
    ESP_LOGI(TAG, "esp_get_free_heap_size : %d ", esp_get_free_heap_size());

    //int *pcTaskName = (int *)pvParameters;
    //char *pcTaskName = (char *)pvParameters;
    pic_param_t test_task_param = (pic_param_t)pvParameters;

    //ESP_LOGI(TAG, "Task_upload_pic Get name  -- [%s]", test_task_param->pic_name);
    ESP_LOGI(TAG, "Task_upload_pic Get app_sub_topic -- [%s]", test_task_param->pic_name_p);

    camera_fb_t *pic = NULL;
    esp_err_t res = ESP_OK;

    char local_response_buffer[2048] = {0};

    //配置服务器相关信息
    esp_http_client_config_t config = {
        .url = "http://aligenie.xuhongv.com/api/upload",
        .method = HTTP_METHOD_POST,
        .event_handler = _http_event_handler,
        .buffer_size = 4096,
        .user_data = local_response_buffer, // Pass address of local buffer to get response
        .buffer_size_tx = 4096 * 5,
        .timeout_ms = 10000,
    };

    //开始拍照
    pic = esp_camera_fb_get();

    if (!pic)
    {
        ESP_LOGE(TAG, "Camera capture failed");
        goto end;
    }
    else
    {
        esp_camera_fb_return(pic);

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

            esp_http_client_handle_t esp_client = esp_http_client_init(&config);

            //设置HTTP请求头为image/jpg表示图片类型
            esp_http_client_set_header(esp_client, "Content-Type", "image/jpg");

            //设置图片名字
            esp_http_client_set_header(esp_client, "Name-Pic", &(test_task_param->pic_name_p));

            //把图片放在body里面
            esp_http_client_set_post_field(esp_client, (const char *)pic->buf, pic->len);

            //开始执行请求服务器
            res = esp_http_client_perform(esp_client);

            //判断是否请求成功
            if (res == ESP_OK)
            {
                ESP_LOGI(TAG, "HTTPS Status = %d", esp_http_client_get_status_code(esp_client));
            }
            else
            {
                ESP_LOGE(TAG, "perform http request %s", esp_err_to_name(res));
            }

            ESP_ERROR_CHECK(esp_http_client_cleanup(esp_client));
        }
    }

end:
{
    ESP_LOGI(TAG, "esp_camera_fb_return End");
    esp_camera_fb_return(pic);
    free(test_task_param);
}
    ESP_LOGI(TAG, "Http End");
    vTaskDelete(NULL);
}

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;

    switch (event->event_id)
    {
    case MQTT_EVENT_CONNECTED:

        esp_mqtt_client_subscribe(client, "/light/deviceIn", 1);
        ESP_LOGI(TAG, "sent subscribe successful, topic: /light/deviceIn");

        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);

        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");

        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);

        // 下发格式 {"action":0,"name":1633075538640}
        cJSON *root_json, *action_json, *name_json;
        root_json = cJSON_Parse(event->data);

        if (!root_json)
        {
            printf("JSON parse error:%s \n\n", cJSON_GetErrorPtr()); //输出json格式错误信息
        }
        else
        {
            action_json = cJSON_GetObjectItem(root_json, "action"); //获取name键对应的值的信息
            if (action_json->type == cJSON_Number)
            {
                printf("action:%d\r\n", action_json->valueint);

                switch (action_json->valueint)
                {
                case 0:
                    name_json = cJSON_GetObjectItem(root_json, "name"); //获取name键对应的值的信息

                    if (name_json->type == cJSON_String)
                    {

                        pic_param_t test_task_param = calloc(1, sizeof(struct Pic_param_t));
                        sprintf(test_task_param->pic_name_p, "%s", name_json->valuestring);
                        //开始拍照上传
                        xTaskCreate(Task_upload_pic, "Task_upload_pic", 8192, test_task_param, 6, NULL);
                    }
                    break;

                default:
                    break;
                }
            }
        }

        cJSON_Delete(root_json); //释放内存

        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
    return ESP_OK;
}

/* 
 * @Description: MQTT参数连接的配置
 * @param: 
 * @return: 
*/
void TaskXMqttRecieve(void *p)
{

    //连接的配置参数
    esp_mqtt_client_config_t mqtt_cfg = {
        .host = "a0je61a.mqtt.iot.gz.baidubce.com", //连接的域名 ，请务必修改为您的
        .port = 1883,                               //端口，请务必修改为您的
        .username = "a0je61a/esp8266",              //用户名，请务必修改为您的
        .password = "sMhrD6kcRFlmIgRF",             //密码，请务必修改为您的
        .client_id = deviceUUID,
        .event_handle = mqtt_event_handler, //设置回调函数
        .keepalive = 120,                   //心跳
        .disable_auto_reconnect = false,    //开启自动重连
        .disable_clean_session = false,     //开启 清除会话
    };
    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_start(client);

    vTaskDelete(NULL);
}

void app_main()
{

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    printf("\n\n-------------------------------- Get Systrm Info------------------------------------------\n");
    //获取IDF版本
    printf("     SDK version:%s\n", esp_get_idf_version());
    //获取芯片可用内存
    printf("     esp_get_free_heap_size : %d  \n", esp_get_free_heap_size());
    //获取从未使用过的最小内存
    printf("     esp_get_minimum_free_heap_size : %d  \n", esp_get_minimum_free_heap_size());
    //获取mac地址（station模式）
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    sprintf(deviceUUID, "%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());

    //initialize the camera
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Camera Init Failed");
    }

    ESP_LOGE(TAG, "camera_config.fb_count = %d", camera_config.fb_count);

    xTaskCreate(&TaskXMqttRecieve, "TaskXMqttRecieve", 1024 * 4, NULL, 5, NULL);
}

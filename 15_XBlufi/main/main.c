/*
 * @Author: your name
 * @Date: 2019-11-26 11:40:54
 * @LastEditTime : 2019-12-21 10:56:33
 * @LastEditors  : Please set LastEditors
 * @Description: In User Settings Edit
 */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <stdio.h>
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_bt.h"
#include "esp_blufi_api.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_err.h"
#include "nvs.h"
#include "xBlufi.h"
#include "esp_bt_device.h"

static const char *TAG = "iot_blufi_test";
static EventGroupHandle_t s_wifi_event_group;
static void TaskXBlufiListener(void *parm);
bool flagBlufiMode = true; //用来标致是否处于配网模式

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    //wifi_mode_t mode;
    ESP_LOGD(TAG, "system_event_handler, event_id: %d", event->event_id);

    switch (event->event_id)
    {
    case SYSTEM_EVENT_STA_START:
        ESP_LOGI(TAG, "SYSTEM_EVENT_STA_START");
        esp_wifi_set_auto_connect(false);
        break;

    case SYSTEM_EVENT_STA_GOT_IP:
        ESP_LOGI(TAG, "SYSTEM_EVENT_STA_GOT_IP");
        XBlufi_notify_got_ip(); //通知获取到了ip
        break;
    case SYSTEM_EVENT_STA_CONNECTED:
        ESP_LOGI(TAG, "SYSTEM_EVENT_STA_CONNECTED");
        XBlufi_notify_connected(event->event_info.connected.bssid); //连接成功路由器，发送路由器的mac地址
        break;

    case SYSTEM_EVENT_STA_DISCONNECTED:
    {
        system_event_sta_disconnected_t *disconnected = &event->event_info.disconnected;
        ESP_LOGI(TAG, "flagBlufiMode: %d ,SYSTEM_EVENT_STA_DISCONNECTED, reason: %d", flagBlufiMode, disconnected->reason);
        //这里处理为了区别是否密码错误而断开非配网指令要求断开连接
        if (flagBlufiMode && (disconnected->reason == WIFI_REASON_CONNECTION_FAIL 
        || disconnected->reason == WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT ))
        {
            esp_wifi_connect();
            XBlufi_notify_connect_fail();
            flagBlufiMode = false;
        }

        break;
    }

    default:
        break;
    }

    return ESP_OK;
}

static void initialise_wifi(void)
{
    tcpip_adapter_init();
    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    xTaskCreate(TaskXBlufiListener, "TaskXBlufiListener", 1024 * 3, NULL, 8, NULL); // 创建任务
}

static void TaskXBlufiListener(void *parm)
{
    xBlufi_start();
    xBlufiMsg msg;
    wifi_config_t wifi_config;
    char blufiName[16];
    bzero(&wifi_config, sizeof(wifi_config_t));
    while (1)
    {
        if (xBlufiReceiveMsg(&msg))
        {
            ESP_LOGI(TAG, "xAirKissReceiveMsg %d", msg.type);
            switch (msg.type)
            {
            case xBlufi_MSG_TYPE_NOTIFY_CONNECT_ACK:
                //停止，释放内存
                // xBlufi_stop_all();
                // vTaskDelete(NULL);
                break;
            case xBlufi_MSG_TYPE_SATRT:
                ESP_LOGI(TAG, "BLUFI deinit finish");
                //初始化后设置任意蓝牙名字，不能大于16个字节
                const uint8_t *addr = esp_bt_dev_get_address(); //查询蓝牙的mac地址，务必要在蓝牙初始化后方可调用！
                sprintf(blufiName, "BLUFI_%02x%02x%02x", addr[0], addr[2], addr[5]);
                XBlufi_notify_set_blufi_name(blufiName, strlen(blufiName));
                break;
            case xBlufi_MSG_TYPE_GET_BSSID: // 成功获取路由器mac地址
                ESP_LOGI(TAG, " get bssid[len:%d]: %s", msg.dataLen, (char *)msg.data);
                break;
            case xBlufi_MSG_TYPE_GET_SSID: // 成功获取路由器名字
                ESP_LOGI(TAG, " get ssid[len:%d]: %s", msg.dataLen, (char *)msg.data);
                memset(wifi_config.sta.ssid, 0, sizeof(wifi_config.sta.ssid));
                memcpy(wifi_config.sta.ssid, (char *)msg.data, msg.dataLen);
                break;
            case xBlufi_MSG_TYPE_GET_PASSWORD: // 成功获取路由器密码
                ESP_LOGI(TAG, " get password[len:%d]: %s", msg.dataLen, (char *)msg.data);
                memset(wifi_config.sta.password, 0, sizeof(wifi_config.sta.password));
                memcpy(wifi_config.sta.password, (char *)msg.data, msg.dataLen);
                break;
            case xBlufi_MSG_TYPE_RECIEVE_CUSTON_DATA: //接收到了自定义的数据
            {
                ESP_LOGI(TAG, " get custon data[len:%d]: %s", msg.dataLen, (char *)msg.data);

                //把接收到的自定义数据，原封不住返回回去给手机
                XBlufi_notify_send_custom_data((uint8_t *)msg.data, msg.dataLen);
                //如果发送是 stop 则断开连接
                if (strcmp("stop", (char *)msg.data) == 0)
                {
                    flagBlufiMode = false;
                    xBlufi_stop_all();
                    vTaskDelete(NULL);
                }
                break;
            }

            case xBlufi_MSG_TYPE_REQ_CONNECT_TO_AP: //手机要求接入热点

                ESP_LOGI(TAG, " start connect , get ssid: %s", (char *)wifi_config.sta.ssid);
                ESP_LOGI(TAG, " start connect , get password: %s", (char *)wifi_config.sta.password);

                //连接路由器
                ESP_ERROR_CHECK(esp_wifi_disconnect());
                ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
                ESP_ERROR_CHECK(esp_wifi_connect());
                flagBlufiMode = true;
                break;
            case xBlufi_MSG_TYPE_REQ_DISCONNECT_TO_AP: //手机要求断开当前热点的接入
                esp_wifi_disconnect();

                break;

            default:
                break;
            }
        }
    }

    vTaskDelete(NULL);
}

/**
 * @description:  程序入口
 * @param {type} 
 * @return: 
 */
void app_main()
{

    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

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
    printf("esp_read_mac(): %02x:%02x:%02x:%02x:%02x:%02x \n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    printf("--------------------------------------------------------------------------\n\n");

    initialise_wifi();
}

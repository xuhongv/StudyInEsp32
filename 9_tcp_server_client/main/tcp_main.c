
#include <errno.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "xuhong_Tcp.h"

//this task establish a TCP connection and receive data from TCP
static void tcp_conn(void *pvParameters)
{
    while (1)
    {

        g_rxtx_need_restart = false;

        ESP_LOGI(TAG, "task tcp_conn...");

        /*wating for connecting to AP*/
        xEventGroupWaitBits(tcp_event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);
        TaskHandle_t tx_rx_task = NULL;

#if TCP_SERVER_CLIENT_OPTION

        ESP_LOGI(TAG, "tcp_server will start after 3s...");
        vTaskDelay(3000 / portTICK_RATE_MS);
        ESP_LOGI(TAG, "create_tcp_server.");
        int socket_ret = create_tcp_server(true);
#else
        ESP_LOGI(TAG, "tcp_client will start after 3s...");
        vTaskDelay(3000 / portTICK_RATE_MS);
        ESP_LOGI(TAG, "create_tcp_Client.");
        int socket_ret = create_tcp_client();
#endif
        if (socket_ret == ESP_FAIL)
        {
            ESP_LOGI(TAG, "create tcp socket error,stop...");
            continue;
        }
        else
        {
            ESP_LOGI(TAG, "create tcp socket succeed...");
        }

        if (pdPASS != xTaskCreate(&recv_data, "recv_data", 4096, NULL, 4, &tx_rx_task))
        {
            ESP_LOGI(TAG, "Recv task create fail!");
        }
        else
        {
            ESP_LOGI(TAG, "Recv task create succeed!");
        }

        double bps;

        while (1)
        {

            vTaskDelay(3000 / portTICK_RATE_MS);

#if TCP_SERVER_CLIENT_OPTION

            if (g_rxtx_need_restart)
            {
                ESP_LOGE(TAG, "tcp server send or receive task encoutner error, need to restart...");

                if (ESP_FAIL != create_tcp_server(false))
                {
                    if (pdPASS != xTaskCreate(&recv_data, "recv_data", 4096, NULL, 4, &tx_rx_task))
                    {
                        ESP_LOGE(TAG, "tcp server Recv task create fail!");
                    }
                    else
                    {
                        ESP_LOGE(TAG, "tcp server Recv task create succeed!");
                    }
                }
            }
#else
            if (g_rxtx_need_restart)
            {
                ESP_LOGI(TAG, "tcp_client will reStart after 3s...");
                vTaskDelay(3000 / portTICK_RATE_MS);
                ESP_LOGI(TAG, "create_tcp_Client...");
                int socket_ret = create_tcp_client();

                if (socket_ret == ESP_FAIL)
                {
                    ESP_LOGE(TAG, "create tcp socket error,stop...");
                    continue;
                }
                else
                {
                    ESP_LOGI(TAG, "create tcp socket succeed...");
                    g_rxtx_need_restart = false;
                }

                if (pdPASS != xTaskCreate(&recv_data, "recv_data", 4096, NULL, 4, &tx_rx_task))
                {
                    ESP_LOGE(TAG, "Recv task create fail!");
                }
                else
                {
                    ESP_LOGI(TAG, "Recv task create succeed!");
                }
            }
#endif
        }
    }

    vTaskDelete(NULL);
}

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

#if TCP_SERVER_CLIENT_OPTION
    ESP_LOGI(TAG, "As a Tcp Server , will start wifi_init_softap...");
    wifi_init_softap();
#else

    ESP_LOGI(TAG, "As a Tcp Client , will start wifi_init_sta...");
    wifi_init_sta();
#endif
    xTaskCreate(&tcp_conn, "tcp_conn", 4096, NULL, 5, NULL);
}

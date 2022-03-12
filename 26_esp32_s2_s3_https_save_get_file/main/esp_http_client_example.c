/* ESP HTTP Client Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "esp_tls.h"
#include "mbedtls/platform.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/esp_debug.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/certs.h"
#include "mbedtls/base64.h"
#include "esp_wnm.h"
#include "esp_rrm.h"
#include "mbedtls/md5.h"
#include "esp_http_client.h"

#include "esp_partition.h"

#define MAX_HTTP_RECV_BUFFER 512
#define MAX_HTTP_OUTPUT_BUFFER 2048

static const char *TAG = "HTTP_CLIENT";

extern const char howsmyssl_com_root_cert_pem_start[] asm("_binary_howsmyssl_com_root_cert_pem_start");
extern const char howsmyssl_com_root_cert_pem_end[] asm("_binary_howsmyssl_com_root_cert_pem_end");

static unsigned char *p_header_md5_value = NULL;
static int Content_Length = 0;
static int binary_file_len = 0;

// sector size of flash
#define FLASH_SECTOR_SIZE (0x1000)

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    static char *output_buffer; // Buffer to store response of http request from event handler
    switch (evt->event_id)
    {
    case HTTP_EVENT_ERROR:
        ESP_LOGI(TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        // ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        if (strcmp(evt->header_key, "Content-MD5") == 0)
        {
            ESP_LOGI(TAG, "HTTP Get Header MD5, key=%s, value=%s", evt->header_key, evt->header_value);
            size_t len;
            int dst_buf_size = 100;
            p_header_md5_value = malloc(dst_buf_size);
            int result = mbedtls_base64_decode(p_header_md5_value, dst_buf_size, &len, (unsigned char *)(evt->header_value), strlen((char *)(evt->header_value)));
            if (result != 0)
            {
                ESP_LOGE(TAG, "fail mbedtls_base64_decode");
            }
        }
        else if (strcmp(evt->header_key, "Content-Length") == 0)
        {
            Content_Length = atoi((evt->header_value));
            ESP_LOGI(TAG, "HTTP Get Content-Length, key=%d, value=%s", Content_Length, evt->header_value);
        }
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
        if (output_buffer != NULL)
        {
            // Response is accumulated in output_buffer. Uncomment the below line to print the accumulated response
            // ESP_LOG_BUFFER_HEX(TAG, output_buffer, output_len);
            free(output_buffer);
            output_buffer = NULL;
        }
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
        int mbedtls_err = 0;
        esp_err_t err = esp_tls_get_and_clear_last_error(evt->data, &mbedtls_err, NULL);
        if (err != 0)
        {
            if (output_buffer != NULL)
            {
                free(output_buffer);
                output_buffer = NULL;
            }
            ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
            ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
        }
        break;
    }
    return ESP_OK;
}

static void http_cleanup(esp_http_client_handle_t client)
{
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
}

static void Task_read_buff(void *pvParameters)
{

    mbedtls_md5_context md5_ctx;
    mbedtls_md5_init(&md5_ctx);
    mbedtls_md5_starts_ret(&md5_ctx);
    unsigned char md5[16] = {0};

    const esp_partition_t *find_partition = NULL;
    find_partition = esp_partition_find_first(0x40, 0x0, NULL);
    if (find_partition == NULL)
    {
        printf("No partition found!\r\n");
    }

    char *flash_read_buff = (char *)malloc(4096);
    if (!flash_read_buff)
    {
        ESP_LOGE(TAG, "Couldn't allocate memory to upgrade data buffer");
    }

    int i = 0;
    //把总的文件大小除于每次取出来的大小，得到取出次数
    int length = binary_file_len / FLASH_SECTOR_SIZE;
    ESP_LOGI(TAG, "MD5 binary_file_len: %d %d ", FLASH_SECTOR_SIZE, length);

    for (int rd_offset = 0; rd_offset < binary_file_len; rd_offset += FLASH_SECTOR_SIZE)
    {
        //判断为最后一次，则获取剩余的buff
        if (++i > length)
        {
            int offLength = binary_file_len - rd_offset;
            ESP_ERROR_CHECK(esp_partition_read(find_partition, rd_offset, flash_read_buff, offLength));
            mbedtls_md5_update(&md5_ctx, (unsigned char *)flash_read_buff, offLength);
        }
        else
        {
            ESP_ERROR_CHECK(esp_partition_read(find_partition, rd_offset, flash_read_buff, FLASH_SECTOR_SIZE));
            mbedtls_md5_update(&md5_ctx, (unsigned char *)flash_read_buff, FLASH_SECTOR_SIZE);
        }
    }

    mbedtls_md5_finish(&md5_ctx, md5);

    ESP_LOGE(TAG, "------------------------------");
    ESP_LOGE(TAG, "Get local save buff MD5 : ");
    esp_log_buffer_hex(TAG, md5, 16);
    ESP_LOGE(TAG, "Get server buff MD5 : ");
    esp_log_buffer_hex(TAG, p_header_md5_value, 16);
    ESP_LOGE(TAG, "------------------------------");

    //校验MD5是否一致
    if (0 == memcmp(md5, p_header_md5_value, sizeof(&p_header_md5_value)))
    {
        // MD5校验成功
        ESP_LOGI(TAG, "MD5 verify success\r\n");
    }
    else
    { // MD5校验失败
        ESP_LOGE(TAG, "MD5 verify fail\r\n");
    }

    free(flash_read_buff);
    mbedtls_md5_free(&md5_ctx);

    vTaskDelete(NULL);
}

static void http_test_task(void *pvParameters)
{

    mbedtls_md5_context md5_ctx;
    mbedtls_md5_init(&md5_ctx);
    mbedtls_md5_starts_ret(&md5_ctx);

    const esp_partition_t *find_partition = NULL;
    // 0x40 对应分区表的 Type
    find_partition = esp_partition_find_first(0x40, 0x0, NULL);
    if (find_partition == NULL)
    {
        printf("No partition found!\r\n");
    }
    esp_http_client_config_t config = {
        .url = "https://aithinker-static.oss-cn-shenzhen.aliyuncs.com/officialwebsite/banner/BLE-Mesh.png", //此图片为 安信可科技提供的阿里云图片
        .event_handler = _http_event_handler,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        .cert_pem = howsmyssl_com_root_cert_pem_start, // 加载证书
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL)
    {
        ESP_LOGE(TAG, "Failed to initialise HTTP connection");
    }

    esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK)
    {
        esp_http_client_cleanup(client);
        ESP_LOGE(TAG, "Failed to open HTTP connection: %d", err);
    }
    esp_http_client_fetch_headers(client);

    ESP_LOGI(TAG, "Please Wait. This may take time");

    char *upgrade_data_buf = (char *)malloc(4096);
    if (!upgrade_data_buf)
    {
        ESP_LOGE(TAG, "Couldn't allocate memory to upgrade data buffer");
    }
    binary_file_len = 0;

    while (1)
    {
        int data_read = esp_http_client_read(client, upgrade_data_buf, FLASH_SECTOR_SIZE);
        if (data_read == 0)
        {
            ESP_LOGI(TAG, "Connection closed,all data received");
            break;
        }
        if (data_read < 0)
        {
            ESP_LOGE(TAG, "Error: SSL data read error");
            break;
        }
        if (data_read > 0)
        {

            ESP_LOGI(TAG, "Written image offAdress %d , length :%d", binary_file_len, FLASH_SECTOR_SIZE);

            if (esp_partition_erase_range(find_partition, binary_file_len, FLASH_SECTOR_SIZE) != ESP_OK)
            {
                ESP_LOGE(TAG, "Erase partition error");
            }

            if (esp_partition_write(find_partition, binary_file_len, (unsigned char *)upgrade_data_buf, data_read) != ESP_OK) // incude '\0'
            {
                ESP_LOGE(TAG, "Write partition data error");
            }

            mbedtls_md5_update(&md5_ctx, (unsigned char *)upgrade_data_buf, data_read);

            binary_file_len += data_read;
        }
    }

    unsigned char md5[16] = {0};

    // win10命令电脑查看 MD5: certutil -hashfile fileName MD5
    mbedtls_md5_finish(&md5_ctx, md5);

    ESP_LOGE(TAG, "------------------------------");
    ESP_LOGE(TAG, "Get local rev buff MD5 : ");
    esp_log_buffer_hex(TAG, md5, 16);
    ESP_LOGE(TAG, "Get server buff MD5 : ");
    esp_log_buffer_hex(TAG, p_header_md5_value, 16);
    ESP_LOGE(TAG, "------------------------------");

    //校验MD5是否一致
    if (0 == memcmp(md5, p_header_md5_value, sizeof(&p_header_md5_value)))
    {
        // MD5校验成功
        ESP_LOGI(TAG, "MD5 verify success\r\n");
    }
    else
    { // MD5校验失败
        ESP_LOGE(TAG, "MD5 verify fail\r\n");
    }

    free(upgrade_data_buf);
    mbedtls_md5_free(&md5_ctx);
    http_cleanup(client);
    // esp_partition_iterator_release(find_partition);
    xTaskCreate(&Task_read_buff, "Task_read_buff", 1024 * 3, NULL, 6, NULL);
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
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());
    ESP_LOGI(TAG, "Connected to AP, begin http example");

    xTaskCreate(&http_test_task, "http_test_task", 1024 * 11, NULL, 6, NULL);
}

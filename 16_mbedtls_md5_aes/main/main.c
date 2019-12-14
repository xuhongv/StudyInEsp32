/*
 * @Author: your name
 * @Date: 2019-11-26 11:40:54
 * @LastEditTime: 2019-12-14 14:38:18
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \esp-iot-solution\examples\empty_project\main\main.c
 */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <stdio.h>
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_err.h"
#include "nvs.h"
#include "mbedtls/pk.h"
#include "mbedtls/sha256.h"
#include "mbedtls/sha1.h"
#include "mbedtls/aes.h"
#include "mbedtls/base64.h"
#include <stdio.h>
#include "esp_system.h"
#include "mbedtls/aes.h"
#include "mbedtls/md5.h"
#include <stddef.h>
#include <stdint.h>
#include "config.h"
#include <string.h>
#include <stdio.h>
#include "esp_system.h"

#define LOG_TAG "AES-DEMO =>"

/**
 * @description: md5加密  32位输出
 * @param {type} 
 * @return: 
 */
static void TaskMd5(void *parm)
{

    unsigned char encrypt[] = "https://blog.csdn.net/xh870189248";
    unsigned char decrypt[16];
    mbedtls_md5_context md5_ctx;
    mbedtls_md5_init(&md5_ctx);
    mbedtls_md5_starts(&md5_ctx);
    mbedtls_md5_update(&md5_ctx, encrypt, strlen((char *)encrypt));
    mbedtls_md5_finish(&md5_ctx, decrypt);
    ESP_LOGI(LOG_TAG, "Md5加密前:[%s] \n md5加密后(32位):", encrypt);

    for (int i = 0; i < 16; i++)
    {
        printf("%02x", decrypt[i]);
    }
    mbedtls_md5_free(&md5_ctx);

    printf("\n");
    ESP_LOGI(LOG_TAG, "--------------------------------------------------------------------------\n\n");
    vTaskDelete(NULL);
}

/**
 * @description: sha1加密
 * @param {type} 
 * @return:     
 */
static void TaskSha1(void *parm)
{
    int i;

    unsigned char decrypt[32];
    const unsigned char encrypt[] = "https://github.com/xuhongv";
    ESP_LOGI(LOG_TAG, "Sha1 要加密数据: %s", encrypt);

    mbedtls_sha1_context sha1_ctx;
    mbedtls_sha1_init(&sha1_ctx);
    mbedtls_sha1_starts(&sha1_ctx);
    mbedtls_sha1_update(&sha1_ctx, encrypt, strlen((char *)encrypt));
    mbedtls_sha1_finish(&sha1_ctx, decrypt);
    mbedtls_sha1_free(&sha1_ctx);

    ESP_LOGI(LOG_TAG, "Sha1 加密后数据:");
    for (i = 0; i < 20; i++)
    {
        printf("%02x", decrypt[i]);
    }

    printf("\n");
    ESP_LOGI(LOG_TAG, "--------------------------------------------------------------------------\n\n");
    vTaskDelete(NULL);
}

/**
 * @description: ECB加密解密：数据块128位 偏移量为0，没填充
 * @param {type} !!!! ECB模式只能实现16字节的明文加解密。 !!!
 * @return: 
 */
static void TaskECB(void *parm)
{

    ESP_LOGI(LOG_TAG, "AES-ECB 加密-数据块(128位)，偏移量为0");

    mbedtls_aes_context aes_ctx;
    //密钥数值
    unsigned char key[16] = {'e', 'c', 'b', 'p', 'a', 's', 's', 'w', 'o', 'r', 'd', '1', '2', '3', '4'};
    //明文空间
    unsigned char plain[16] = "csdn-xuhong";
    //解密后明文的空间
    unsigned char dec_plain[16] = {0};
    //密文空间
    unsigned char cipher[16] = {0};

    mbedtls_aes_init(&aes_ctx);
    //设置加密密钥
    mbedtls_aes_setkey_enc(&aes_ctx, key, 128);
    ESP_LOGI(LOG_TAG, "要加密的数据: %s", plain);
    ESP_LOGI(LOG_TAG, "加密的密码: %s", key);
    mbedtls_aes_crypt_ecb(&aes_ctx, MBEDTLS_AES_ENCRYPT, plain, cipher);
    ESP_LOGI(LOG_TAG, "加密结果，二进制表示: ");
    for (int loop = 0; loop < 16; loop++)
        printf("%02x", cipher[loop]);
    printf("\r\n");

    //设置解密密钥
    mbedtls_aes_setkey_dec(&aes_ctx, key, 128);
    mbedtls_aes_crypt_ecb(&aes_ctx, MBEDTLS_AES_DECRYPT, cipher, dec_plain);
    ESP_LOGI(LOG_TAG, "解密后的数据: %s", dec_plain);
    mbedtls_aes_free(&aes_ctx);

    ESP_LOGI(LOG_TAG, "--------------------------------------------------------------------------\n\n");
    vTaskDelete(NULL);
}

/**
 * @description: SHA-256 或 SHA-244
 * @param {type} 
 * @return: 
 */
static void TaskSha256(void *parm)
{
    int i;

    unsigned char decrypt[32];
    const unsigned char encrypt[] = "https://github.com/xuhongv";
    // sha256/224
    ESP_LOGI(LOG_TAG, "Sha256 要加密数据: %s", encrypt);
    mbedtls_sha256_context sha256_ctx;
    mbedtls_sha256_init(&sha256_ctx);
    mbedtls_sha256_starts(&sha256_ctx, 0); // 0表示传sha256 ， 1 表示传SHA-244
    mbedtls_sha256_update(&sha256_ctx, encrypt, strlen((char *)encrypt));
    mbedtls_sha256_finish(&sha256_ctx, decrypt);
    ESP_LOGI(LOG_TAG, "Sha256 加密后数据: ");
    for (i = 0; i < 32; i++)
    {
        printf("%02x", decrypt[i]);
    }
    mbedtls_sha256_free(&sha256_ctx);
    printf("\n");
    ESP_LOGI(LOG_TAG, "--------------------------------------------------------------------------\n\n");
    vTaskDelete(NULL);
}

/**
 * @description: 
 * @param {type} 
 * @return: 
 */
static void TaskAESCBC(void *parm)
{
    int i;

    mbedtls_aes_context aes_ctx;

    //密钥数值
    unsigned char key[16] = {'c', 'b', 'c', 'p', 'a', 's', 's', 'w', 'o', 'r', 'd', '1', '2', '3', '4'};
    //iv
    unsigned char iv[16];

    //明文空间
    unsigned char plain[64] = "https://github.com/xuhongv";
    //解密后明文的空间
    unsigned char dec_plain[64] = {0};
    //密文空间
    unsigned char cipher[64] = {0};

    mbedtls_aes_init(&aes_ctx);

    //设置加密密钥
    printf("plain:%s\r\n", plain);
    mbedtls_aes_setkey_enc(&aes_ctx, key, 128);
    for (i = 0; i < 16; i++)
    {
        iv[i] = 0x01;
    }
    mbedtls_aes_crypt_cbc(&aes_ctx, MBEDTLS_AES_ENCRYPT, 64, iv, plain, cipher);
    printf("cipher:\r\n");

    for (i = 0; i < 64; i++)
    {
        printf("%02x", cipher[i]);
    }
    printf("\r\n");
    //设置解密密钥
    mbedtls_aes_setkey_dec(&aes_ctx, key, 128);
    for (i = 0; i < 16; i++)
    {
        iv[i] = 0x01;
    }
    mbedtls_aes_crypt_cbc(&aes_ctx, MBEDTLS_AES_DECRYPT, 64, iv, cipher, dec_plain);
    printf("dec_plain:%s\r\n", dec_plain);
    printf("\r\n");
    mbedtls_aes_free(&aes_ctx);

    vTaskDelete(NULL);
}

/**
 * @description: 程序入口
 * @param {type} 
 * @return: 
 */
void app_main(void)
{

    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    printf("\n\n-------------------------------- Get Systrm Info Start------------------------------------------\n");
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
    printf("\n\n-------------------------------- Get Systrm Info End------------------------------------------\n");

    xTaskCreate(TaskECB, "TaskECB", 1024 * 2, NULL, 8, NULL); // 创建任务
   
    //xTaskCreate(TaskSha1, "TaskSha1", 1024 * 2, NULL, 8, NULL); // 创建任务
   
    //xTaskCreate(TaskSha256, "TaskSha256", 1024 * 2, NULL, 8, NULL); // 创建任务
   
    //xTaskCreate(TaskAESCBC, "TaskAESCBC", 1024 * 2, NULL, 8, NULL); // 创建任务
   
    //xTaskCreate(TaskMd5, "TaskMd5", 1024 * 2, NULL, 8, NULL); // 创建任务
}

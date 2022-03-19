/*
 * @Author: https://github.com/xuhongv
 * @Date: 2022-03-18 07:41:55
 * @LastEditTime: 2022-03-19 14:06:52
 * @LastEditors: Please set LastEditors
 * @Description: ota https 头文件
 */

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum
    {
        OTA_CERT_SSL_VERIFY_NONE = 0, // 不校验证书,直接通过
        OTA_CERT_SSL_VERIFY_OPTIONAL, // 校验证书并给出结果，由用户决定是否继续请求
        OTA_CERT_SSL_VERIFY_REQUIRED, // 校验证书并给出结果，必须证书通过才继续请求
    } ota_ssl_cert_verify_set_t;

    /**
     * @brief otas_http_client_config configuration
     */
    typedef struct
    {
        const char *url;
        ota_ssl_cert_verify_set_t cert_set;
        bool skip_ssl_cert_set; //是否跳过证书认证，仅当 ota_ssl_cert_verify_set_t 为 OTA_CERT_SSL_VERIFY_OPTIONAL有效

        int url_length;

    } otas_http_client_config;

    typedef struct
    {

        char path[100];

        char version[20];
        int port;
        char token[80];
        char host[20];

    } ota_info;
    ota_info ota_info_item;

    esp_err_t start_https_ota(const otas_http_client_config *config);

#ifdef __cplusplus
}
#endif

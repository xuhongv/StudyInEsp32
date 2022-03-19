# 【OTA HTTP(S)】无线远程升级支持跳过证书升级

本工程由半颗心脏编程并开源，使用的主要协议栈有 mbedtls 和 esp_ota 。

源码地址：https://github.com/xuhongv/StudyInEsp32/tree/master/27_ota_https_skip_cert_option

# 硬件要求

安信可在售 ESP32/S3/C3 模组

# 软件版本

esp-idf  版本:  

```
commit 8ffddf53bc9cb0c36d1949476e244b202f3b42d2 (origin/release/v4.3)
```
# 如何使用此Demo
- 先配置路由器信息。
- 修改ota文件的URL，并且修改是否需要跳过证书。
# API 说明

见注释：

```
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
    
 esp_err_t start_https_ota(const otas_http_client_config *config);
```

# 常见问题 FAQ

### 1. 如何替换证书？

请把域名证书替换 https_ota\cert\server_root_cert.pem 里面内容即可。

### 2. 如何支持HTTPS连接，但不做证书校验？

参数开始时候，请把 cert_set 设置为 OTA_CERT_SSL_VERIFY_OPTIONAL ，把 skip_ssl_cert_set设置为 false 。
```
 .cert_set = OTA_CERT_SSL_VERIFY_OPTIONAL,
 .skip_ssl_cert_set = false,
```





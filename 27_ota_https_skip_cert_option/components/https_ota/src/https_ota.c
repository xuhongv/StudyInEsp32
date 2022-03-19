// Copyright 2017-2018 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <esp_ota_ops.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <http_parser.h>
#include <sys/socket.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "mbedtls/platform.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/esp_debug.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/certs.h"
#include "esp_crt_bundle.h"
#include "esp_ota_ops.h"
#include "https_ota.h"

static const char *TAG = "ota:";

static const char *REQUEST_FORMAT = "GET  %s  HTTP/1.0\r\n"
                                    "Host:  %s \r\n"
                                    "User-Agent: csdn xuhongv\r\n"
                                    "Content-Type: application/json\r\n"
                                    "\r\n";

extern const uint8_t server_root_cert_pem_start[] asm("_binary_server_root_cert_pem_start");
extern const uint8_t server_root_cert_pem_end[] asm("_binary_server_root_cert_pem_end");

#define BUFFSIZE 1024 * 8 // 8KB

typedef enum esp_ota_firm_state
{
    ESP_OTA_INIT = 0,
    ESP_OTA_PREPARE,
    ESP_OTA_START,
    ESP_OTA_RECVED,
    ESP_OTA_FINISH,
} esp_ota_firm_state_t;

typedef struct esp_ota_firm
{
    uint8_t ota_num;
    uint8_t update_ota_num;
    esp_ota_firm_state_t state;
    size_t content_len;
    size_t read_bytes;
    size_t write_bytes;
    size_t ota_size;
    size_t ota_offset;
    const char *buf;
    size_t bytes;
} esp_ota_firm_t;

/*an ota data write buffer ready to write to the flash*/
static char ota_write_data[BUFFSIZE + 1] = {0};
/* an image total length*/
static int binary_file_length = 0;

/*read buffer by byte still delim ,return read bytes counts*/
static int read_until(const char *buffer, char delim, int len)
{
    //  /*TODO: delim check,buffer check,further: do an buffer length limited*/
    int i = 0;
    while (buffer[i] != delim && i < len)
    {
        ++i;
    }
    return i + 1;
}

static void __attribute__((noreturn)) task_fatal_error()
{
    ESP_LOGE(TAG, "Exiting task due to fatal error...");

    (void)vTaskDelete(NULL);

    while (1)
    {
        ;
    }
}

bool _esp_ota_firm_parse_http(esp_ota_firm_t *ota_firm, const char *text, size_t total_len, size_t *parse_len)
{

    /* i means current position */
    int i = 0, i_read_len = 0;
    char *ptr = NULL, *ptr2 = NULL;
    char length_str[32];

    while (text[i] != 0 && i < total_len)
    {
        if (ota_firm->content_len == 0 && (ptr = (char *)strstr(text, "Content-Length")) != NULL)
        {
            ptr += 16;
            ptr2 = (char *)strstr(ptr, "\r\n");
            memset(length_str, 0, sizeof(length_str));
            memcpy(length_str, ptr, ptr2 - ptr);
            ota_firm->content_len = atoi(length_str);
            ota_firm->ota_size = ota_firm->content_len;
            ota_firm->ota_offset = 0;
            ESP_LOGI(TAG, "parse Content-Length:%d, ota_size %d", ota_firm->content_len, ota_firm->ota_size);
        }

        i_read_len = read_until(&text[i], '\n', total_len - i);

        if (i_read_len > total_len - i)
        {
            ESP_LOGE(TAG, "recv malformed http header");
            task_fatal_error();
        }

        // if resolve \r\n line, http header is finished
        if (i_read_len == 2)
        {
            if (ota_firm->content_len == 0)
            {
                ESP_LOGE(TAG, "did not parse Content-Length item");
                task_fatal_error();
            }

            *parse_len = i + 2;

            return true;
        }

        i += i_read_len;
    }

    return false;
}

static size_t esp_ota_firm_do_parse_msg(esp_ota_firm_t *ota_firm, const char *in_buf, size_t in_len)
{
    size_t tmp;
    size_t parsed_bytes = in_len;

    switch (ota_firm->state)
    {
    case ESP_OTA_INIT:
        if (_esp_ota_firm_parse_http(ota_firm, in_buf, in_len, &tmp))
        {
            ota_firm->state = ESP_OTA_PREPARE;
            ESP_LOGD(TAG, "Http parse %d bytes", tmp);
            parsed_bytes = tmp;
        }
        break;
    case ESP_OTA_PREPARE:
        ota_firm->read_bytes += in_len;

        if (ota_firm->read_bytes >= ota_firm->ota_offset)
        {
            ota_firm->buf = &in_buf[in_len - (ota_firm->read_bytes - ota_firm->ota_offset)];
            ota_firm->bytes = ota_firm->read_bytes - ota_firm->ota_offset;
            ota_firm->write_bytes += ota_firm->read_bytes - ota_firm->ota_offset;
            ota_firm->state = ESP_OTA_START;
            ESP_LOGD(TAG, "Receive %d bytes and start to update", ota_firm->read_bytes);
            ESP_LOGD(TAG, "Write %d total %d", ota_firm->bytes, ota_firm->write_bytes);
        }

        break;
    case ESP_OTA_START:
        if (ota_firm->write_bytes + in_len > ota_firm->ota_size)
        {
            ota_firm->bytes = ota_firm->ota_size - ota_firm->write_bytes;
            ota_firm->state = ESP_OTA_RECVED;
        }
        else
            ota_firm->bytes = in_len;

        ota_firm->buf = in_buf;

        ota_firm->write_bytes += ota_firm->bytes;

        ESP_LOGD(TAG, "Write %d total %d", ota_firm->bytes, ota_firm->write_bytes);

        break;
    case ESP_OTA_RECVED:
        parsed_bytes = 0;
        ota_firm->state = ESP_OTA_FINISH;
        break;
    default:
        parsed_bytes = 0;
        ESP_LOGD(TAG, "State is %d", ota_firm->state);
        break;
    }

    return parsed_bytes;
}

static void esp_ota_firm_parse_msg(esp_ota_firm_t *ota_firm, const char *in_buf, size_t in_len)
{
    size_t parse_bytes = 0;
    ESP_LOGD(TAG, "Input %d bytes", in_len);
    do
    {
        size_t bytes = esp_ota_firm_do_parse_msg(ota_firm, in_buf + parse_bytes, in_len - parse_bytes);
        ESP_LOGD(TAG, "Parse %d bytes", bytes);
        if (bytes)
            parse_bytes += bytes;
    } while (parse_bytes != in_len);
}

static inline int esp_ota_firm_is_finished(esp_ota_firm_t *ota_firm)
{
    return (ota_firm->state == ESP_OTA_FINISH || ota_firm->state == ESP_OTA_RECVED);
}

static inline int esp_ota_firm_can_write(esp_ota_firm_t *ota_firm)
{
    return (ota_firm->state == ESP_OTA_START || ota_firm->state == ESP_OTA_RECVED);
}

static inline const char *esp_ota_firm_get_write_buf(esp_ota_firm_t *ota_firm)
{
    return ota_firm->buf;
}

static inline size_t esp_ota_firm_get_write_bytes(esp_ota_firm_t *ota_firm)
{
    return ota_firm->bytes;
}

static void esp_ota_firm_init(esp_ota_firm_t *ota_firm, const esp_partition_t *update_partition)
{
    memset(ota_firm, 0, sizeof(esp_ota_firm_t));
    ota_firm->state = ESP_OTA_INIT;
    ota_firm->update_ota_num = update_partition->subtype - ESP_PARTITION_SUBTYPE_APP_OTA_0;
    ESP_LOGI(TAG, "Totoal OTA number %d update to %d part", ota_firm->ota_num, ota_firm->update_ota_num);
}

static int get_port(const char *url, struct http_parser_url *u)
{
    if (u->field_data[UF_PORT].len)
    {
        return strtol(&url[u->field_data[UF_PORT].off], NULL, 10);
    }
    else
    {
        if (strncasecmp(&url[u->field_data[UF_SCHEMA].off], "http", u->field_data[UF_SCHEMA].len) == 0)
        {
            return 80;
        }
        else if (strncasecmp(&url[u->field_data[UF_SCHEMA].off], "https", u->field_data[UF_SCHEMA].len) == 0)
        {
            return 443;
        }
    }
    return 0;
}

esp_err_t start_https_ota(const otas_http_client_config *config)
{

    char buf[1024], s_port[6];
    int len = 0, flags = 0, ret = 0;
    esp_err_t err = ESP_FAIL;

    if (config)
    {
        ESP_LOGI(TAG, "get length: %d", (config->url_length));
        ESP_LOGI(TAG, "get url: %s", (config->url));
    }
    else
        ESP_LOGI(TAG, "get url is NULL");

    struct http_parser_url u;
    http_parser_url_init(&u);
    http_parser_parse_url(config->url, config->url_length, 0, &u);
    char *use_host = strndup(&(config->url[u.field_data[UF_HOST].off]), u.field_data[UF_HOST].len);
    sprintf(s_port, "%d", get_port(config->url, &u));

    ESP_LOGI(TAG, "host:%s", use_host);
    ESP_LOGI(TAG, "path: %s", &(config->url[u.field_data[UF_PATH].off]));
    ESP_LOGI(TAG, "port: %s", s_port);

    /********************************************/

    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_ssl_context ssl;
    mbedtls_x509_crt cacert;
    mbedtls_ssl_config conf;
    mbedtls_net_context server_fd;

    char *REQUEST = (char *)malloc(512);
    sprintf(REQUEST, REQUEST_FORMAT, &(config->url[u.field_data[UF_PATH].off]), use_host);

    ESP_LOGI(TAG, "http header: \n%s\n", REQUEST);

    /********************    OTA  start set        ************************/
    /* update handle : set by esp_ota_begin(), must be freed via esp_ota_end() */
    /* update handle : set by esp_ota_begin(), must be freed via esp_ota_end() */

    esp_ota_handle_t update_handle = 0;
    const esp_partition_t *update_partition = NULL;

    ESP_LOGI(TAG, "Starting OTA example");

    const esp_partition_t *configured = esp_ota_get_boot_partition();
    const esp_partition_t *running = esp_ota_get_running_partition();

    if (configured != running)
    {
        ESP_LOGW(TAG, "Configured OTA boot partition at offset 0x%08x, but running from offset 0x%08x", configured->address, running->address);
        ESP_LOGW(TAG, "(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)");
    }
    ESP_LOGI(TAG, "Running partition type %d subtype %d (offset 0x%08x)", running->type, running->subtype, running->address);
    ESP_LOGI(TAG, "initting ... this may take a while.)");

    /******************    OTA  start  end        **************************/

    mbedtls_ssl_init(&ssl);
    mbedtls_x509_crt_init(&cacert);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_ssl_config_init(&conf);
    mbedtls_entropy_init(&entropy);

    if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                                     NULL, 0)) != 0)
    {
        ESP_LOGE(TAG, "mbedtls_ctr_drbg_seed returned %d", ret);
        abort();
    }

    ESP_LOGI(TAG, "Loading the CA root certificate...");

    ret = mbedtls_x509_crt_parse(&cacert, server_root_cert_pem_start, server_root_cert_pem_end - server_root_cert_pem_start);

    if (ret < 0)
    {
        ESP_LOGE(TAG, "mbedtls_x509_crt_parse returned -0x%x\n\n", -ret);
        abort();
    }

    ESP_LOGI(TAG, "Setting hostname for TLS session...");

    /* Hostname set here should match CN in server certificate */
    if ((ret = mbedtls_ssl_set_hostname(&ssl, use_host)) != 0)
    {
        ESP_LOGE(TAG, "mbedtls_ssl_set_hostname returned -0x%x", -ret);
        abort();
    }

    ESP_LOGI(TAG, "Setting up the SSL/TLS structure...");

    if ((ret = mbedtls_ssl_config_defaults(&conf, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT)) != 0)
    {
        ESP_LOGE(TAG, "mbedtls_ssl_config_defaults returned %d", ret);
        goto exit;
    }

    /* MBEDTLS_SSL_VERIFY_OPTIONAL is bad for security, in this example it will print
      a warning if CA verification fails but it will continue to connect.
      You should consider using MBEDTLS_SSL_VERIFY_REQUIRED in your own code.
   */
    mbedtls_ssl_conf_authmode(&conf, config->cert_set);
    if (config->cert_set != OTA_CERT_SSL_VERIFY_NONE)
    {
        mbedtls_ssl_conf_ca_chain(&conf, &cacert, NULL);
    }

    mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);

    if ((ret = mbedtls_ssl_setup(&ssl, &conf)) != 0)
    {
        ESP_LOGE(TAG, "mbedtls_ssl_setup returned -0x%x\n\n", -ret);
        goto exit;
    }
    mbedtls_net_init(&server_fd);

    ESP_LOGI(TAG, "Connecting to %s:%s...", use_host, s_port);

    if ((ret = mbedtls_net_connect(&server_fd, use_host, s_port, MBEDTLS_NET_PROTO_TCP)) != 0)
    {
        ESP_LOGE(TAG, "mbedtls_net_connect returned -%x", -ret);
        goto exit;
    }

    ESP_LOGI(TAG, "Connected.");

    mbedtls_ssl_set_bio(&ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv, NULL);

    ESP_LOGI(TAG, "Performing the SSL/TLS handshake...");

    while ((ret = mbedtls_ssl_handshake(&ssl)) != 0)
    {
        if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
        {
            ESP_LOGE(TAG, "mbedtls_ssl_handshake returned -0x%x", -ret);
            goto exit;
        }
    }

    if (config->cert_set != OTA_CERT_SSL_VERIFY_NONE)
    {
        ESP_LOGI(TAG, "Verifying peer X.509 certificate...");
        //获取校验证书结果
        if (mbedtls_ssl_get_verify_result(&ssl) != 0)
        {
            /* In real life, we probably want to close connection if ret != 0 */
            ESP_LOGW(TAG, "Failed to verify peer certificate!");
            bzero(buf, sizeof(buf));
            mbedtls_x509_crt_verify_info(buf, sizeof(buf), "  ! ", flags);
            ESP_LOGW(TAG, "verification flags: %d , info: %s", flags, buf);
            if ((config->cert_set == OTA_CERT_SSL_VERIFY_OPTIONAL && config->skip_ssl_cert_set) || config->cert_set == OTA_CERT_SSL_VERIFY_REQUIRED)
            {
                err = ESP_FAIL;
                goto exit;
            }
        }
        else
        {
            ESP_LOGI(TAG, "Certificate verified success.");
        }
    }
    else
    {
        ESP_LOGI(TAG, "Verifying peer X.509 not certificate...");
    }

    ESP_LOGI(TAG, "Cipher suite is %s", mbedtls_ssl_get_ciphersuite(&ssl));

    ESP_LOGI(TAG, "Writing HTTP request...");

    size_t written_bytes = 0;

    do
    {
        ret = mbedtls_ssl_write(&ssl, (const unsigned char *)REQUEST + written_bytes, strlen(REQUEST) - written_bytes);
        if (ret >= 0)
        {
            ESP_LOGI(TAG, "%d bytes written", ret);
            written_bytes += ret;
        }
        else if (ret != MBEDTLS_ERR_SSL_WANT_WRITE && ret != MBEDTLS_ERR_SSL_WANT_READ)
        {
            ESP_LOGE(TAG, "mbedtls_ssl_write returned -0x%x", -ret);
            goto exit;
        }
    } while (written_bytes < strlen(REQUEST));

    update_partition = esp_ota_get_next_update_partition(NULL);

    ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%x", update_partition->subtype, update_partition->address);
    assert(update_partition != NULL);

    err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_ota_begin failed, error=%d", err);
        task_fatal_error();
    }
    ESP_LOGI(TAG, "esp_ota_begin succeeded");

    esp_ota_firm_t ota_firm;
    esp_ota_firm_init(&ota_firm, update_partition);

    ESP_LOGI(TAG, "Reading HTTP response...");

    do
    {
        memset(ota_write_data, 0, BUFFSIZE);

        len = mbedtls_ssl_read(&ssl, (unsigned char *)buf, BUFFSIZE);

        if (len == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE)
            continue;

        if (len == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY)
        {
            len = 0;
            break;
        }
        else if (len > 0)
        {

            esp_ota_firm_parse_msg(&ota_firm, buf, len);

            if (!esp_ota_firm_can_write(&ota_firm))
                continue;

            memcpy(ota_write_data, esp_ota_firm_get_write_buf(&ota_firm), esp_ota_firm_get_write_bytes(&ota_firm));
            len = esp_ota_firm_get_write_bytes(&ota_firm);

            err = esp_ota_write(update_handle, (const void *)ota_write_data, len);
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "Error: esp_ota_write failed! err=0x%x", err);
                task_fatal_error();
            }
            binary_file_length += len;
            ESP_LOGI(TAG, "Have written image length %d", binary_file_length);
        }
        else if (len < 0)
        {
            ESP_LOGE(TAG, "mbedtls_ssl_read returned -0x%x", -ret);
            break;
        }
        else if (len == 0)
        {
            ESP_LOGI(TAG, "connection closed");
            break;
        }

        if (esp_ota_firm_is_finished(&ota_firm))
            break;

    } while (1);

    mbedtls_ssl_close_notify(&ssl);

    ESP_LOGI(TAG, "Total Write binary data length : %d", binary_file_length);

    if (esp_ota_end(update_handle) != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_ota_end failed!");
        task_fatal_error();
    }
    err = esp_ota_set_boot_partition(update_partition);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed! err=0x%x", err);
        task_fatal_error();
        goto exit;
    }

exit:

    mbedtls_ssl_session_reset(&ssl);
    mbedtls_net_free(&server_fd);

    free(REQUEST);
    free(use_host);

    return err;
}

/* tcp_perf Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <string.h>
#include <sys/socket.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"

#include "xuhong_Tcp.h"

/* FreeRTOS event group to signal when we are connected to wifi */
EventGroupHandle_t tcp_event_group;

/*socket*/
static int server_socket = 0;
static struct sockaddr_in server_addr;
static struct sockaddr_in client_addr;
static unsigned int socklen = sizeof(client_addr);
static int connect_socket = 0;
bool g_rxtx_need_restart = false;

int g_total_data = 0;

#if EXAMPLE_ESP_TCP_PERF_TX && EXAMPLE_ESP_TCP_DELAY_INFO

int g_total_pack = 0;
int g_send_success = 0;
int g_send_fail = 0;
int g_delay_classify[5] = {0};

#endif /*EXAMPLE_ESP_TCP_PERF_TX && EXAMPLE_ESP_TCP_DELAY_INFO*/

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch (event->event_id)
    {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        esp_wifi_connect();
        xEventGroupClearBits(tcp_event_group, WIFI_CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_STA_CONNECTED:
        xEventGroupSetBits(tcp_event_group, WIFI_CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        ESP_LOGI(TAG, "got ip:%s\n",
                 ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
        xEventGroupSetBits(tcp_event_group, WIFI_CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_AP_STACONNECTED:
        ESP_LOGI(TAG, "station:" MACSTR " join,AID=%d\n",
                 MAC2STR(event->event_info.sta_connected.mac),
                 event->event_info.sta_connected.aid);
        xEventGroupSetBits(tcp_event_group, WIFI_CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_AP_STADISCONNECTED:
        ESP_LOGI(TAG, "station:" MACSTR "leave,AID=%d\n",
                 MAC2STR(event->event_info.sta_disconnected.mac),
                 event->event_info.sta_disconnected.aid);
        g_rxtx_need_restart = true;
        xEventGroupClearBits(tcp_event_group, WIFI_CONNECTED_BIT);
        break;
    default:
        break;
    }
    return ESP_OK;
}

//send data
void send_data(void *pvParameters)
{
    int len = 0;
    char *databuff = (char *)malloc(EXAMPLE_DEFAULT_PKTSIZE * sizeof(char));
    memset(databuff, EXAMPLE_PACK_BYTE_IS, EXAMPLE_DEFAULT_PKTSIZE);
    vTaskDelay(100 / portTICK_RATE_MS);
    ESP_LOGI(TAG, "start sending...");

    while (1)
    {
        int to_write = EXAMPLE_DEFAULT_PKTSIZE;

        while (to_write > 0)
        {
            len = send(connect_socket, databuff + (EXAMPLE_DEFAULT_PKTSIZE - to_write), to_write, 0);
            if (len > 0)
            {
                g_total_data += len;
                to_write -= len;
            }
            else
            {
                int err = get_socket_error_code(connect_socket);

                if (err != ENOMEM)
                {
                    show_socket_error_reason("send_data", connect_socket);
                    break;
                }
            }
        }

#if EXAMPLE_ESP_TCP_PERF_TX && EXAMPLE_ESP_TCP_DELAY_INFO
        gettimeofday(&tv_finish, NULL);
#endif /*EXAMPLE_ESP_TCP_PERF_TX && EXAMPLE_ESP_TCP_DELAY_INFO*/
        if (g_total_data > 0)
        {
#if EXAMPLE_ESP_TCP_PERF_TX && EXAMPLE_ESP_TCP_DELAY_INFO
            g_send_success++;
            send_delay_ms = (tv_finish.tv_sec - tv_start.tv_sec) * 1000 + (tv_finish.tv_usec - tv_start.tv_usec) / 1000;
            if (send_delay_ms < 30)
            {
                g_delay_classify[0]++;
            }
            else if (send_delay_ms < 100)
            {
                g_delay_classify[1]++;
            }
            else if (send_delay_ms < 300)
            {
                g_delay_classify[2]++;
            }
            else if (send_delay_ms < 1000)
            {
                g_delay_classify[3]++;
            }
            else
            {
                g_delay_classify[4]++;
            }
#endif /*EXAMPLE_ESP_TCP_PERF_TX && EXAMPLE_ESP_TCP_DELAY_INFO*/
        }
        else
        {
            break;
        }
    }
    g_rxtx_need_restart = true;
    free(databuff);
    vTaskDelete(NULL);
}

//receive data
void recv_data(void *pvParameters)
{
    int len = 0;

    char databuff[1024];

    while (1)
    {
        //每次接收都要清空接收数组
        memset(databuff, 0x00, sizeof(databuff));
        len = recv(connect_socket, databuff, sizeof(databuff), 0);
        g_rxtx_need_restart = false;
        if (len > 0)
        {
            g_total_data += len;
            //打印接收到的数组
            ESP_LOGI(TAG, "recvData: %s\n", databuff);
            //原路返回，不指定某个客户端
            send(connect_socket, databuff, sizeof(databuff), 0);
            //sendto(connect_socket, databuff , sizeof(databuff), 0, (struct sockaddr *) &remote_addr,sizeof(remote_addr));
        }
        else
        {
            show_socket_error_reason("recv_data", connect_socket);
            g_rxtx_need_restart = true;
#if !TCP_SERVER_CLIENT_OPTION
            break;
#endif
        }
    }

    close_socket();
    g_rxtx_need_restart = true;
    vTaskDelete(NULL);
}

esp_err_t create_tcp_server(bool isCreatServer)
{

    if (isCreatServer)
    {
        ESP_LOGI(TAG, "server socket....,port=%d", TCP_PORT);
        server_socket = socket(AF_INET, SOCK_STREAM, 0);

        if (server_socket < 0)
        {
            show_socket_error_reason("create_server", server_socket);
            return ESP_FAIL;
        }

        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(TCP_PORT);
        server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

        if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        {
            show_socket_error_reason("bind_server", server_socket);
            close(server_socket);
            return ESP_FAIL;
        }
    }

    if (listen(server_socket, 5) < 0)
    {
        show_socket_error_reason("listen_server", server_socket);
        close(server_socket);
        return ESP_FAIL;
    }

    connect_socket = accept(server_socket, (struct sockaddr *)&client_addr, &socklen);

    if (connect_socket < 0)
    {
        show_socket_error_reason("accept_server", connect_socket);
        close(server_socket);
        return ESP_FAIL;
    }

    /*connection established，now can send/recv*/
    ESP_LOGI(TAG, "tcp connection established!");
    return ESP_OK;
}

//创建TCP客户端连接到指定的服务器
esp_err_t create_tcp_client()
{

    ESP_LOGI(TAG, "will connect gateway ssid : %s port:%d\n",
             TCP_SERVER_ADRESS, TCP_PORT);

    connect_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (connect_socket < 0)
    {
        show_socket_error_reason("create client", connect_socket);
        return ESP_FAIL;
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(TCP_PORT);
    server_addr.sin_addr.s_addr = inet_addr(TCP_SERVER_ADRESS);
    ESP_LOGI(TAG, "connectting server...");
    if (connect(connect_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        show_socket_error_reason("client connect", connect_socket);
        ESP_LOGE(TAG, "connect failed!");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "connect success!");
    return ESP_OK;
}

//wifi_init_sta
void wifi_init_sta()
{
    tcp_event_group = xEventGroupCreate();

    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = GATEWAY_SSID,
            .password = GATEWAY_PAS},
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished.");
    ESP_LOGI(TAG, "connect to ap SSID:%s password:%s \n",
             GATEWAY_SSID, GATEWAY_PAS);
}

//wifi_init_softap
void wifi_init_softap()
{
    tcp_event_group = xEventGroupCreate();

    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = SOFT_AP_SSID,
            .ssid_len = 0,
            .max_connection = SOFT_AP_MAX_CONNECT,
            .password = SOFT_AP_PAS,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK},
    };
    if (strlen(EXAMPLE_DEFAULT_PWD) == 0)
    {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "SoftAP set finish:%s pas:%s \n",
             EXAMPLE_DEFAULT_SSID, EXAMPLE_DEFAULT_PWD);
}

int get_socket_error_code(int socket)
{
    int result;
    u32_t optlen = sizeof(int);
    int err = getsockopt(socket, SOL_SOCKET, SO_ERROR, &result, &optlen);
    if (err == -1)
    {
        ESP_LOGE(TAG, "getsockopt failed:%s", strerror(err));
        return -1;
    }
    return result;
}

int show_socket_error_reason(const char *str, int socket)
{
    int err = get_socket_error_code(socket);

    if (err != 0)
    {
        ESP_LOGW(TAG, "%s socket error %d %s", str, err, strerror(err));
    }

    return err;
}

int check_working_socket()
{
    int ret;
#if EXAMPLE_ESP_TCP_MODE_SERVER
    ESP_LOGD(TAG, "check server_socket");
    ret = get_socket_error_code(server_socket);
    if (ret != 0)
    {
        ESP_LOGW(TAG, "server socket error %d %s", ret, strerror(ret));
    }
    if (ret == ECONNRESET)
    {
        return ret;
    }
#endif
    ESP_LOGD(TAG, "check connect_socket");
    ret = get_socket_error_code(connect_socket);
    if (ret != 0)
    {
        ESP_LOGW(TAG, "connect socket error %d %s", ret, strerror(ret));
    }
    if (ret != 0)
    {
        return ret;
    }
    return 0;
}

void close_socket()
{
    close(connect_socket);
    close(server_socket);
}

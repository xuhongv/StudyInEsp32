
#ifndef __TCP_PERF_H__
#define __TCP_PERF_H__



#ifdef __cplusplus
extern "C" {
#endif


/*test options*/
#define EXAMPLE_ESP_WIFI_MODE_AP CONFIG_TCP_PERF_WIFI_MODE_AP
#define EXAMPLE_ESP_TCP_MODE_SERVER CONFIG_TCP_PERF_SERVER 
#define EXAMPLE_ESP_TCP_PERF_TX CONFIG_TCP_PERF_TX
#define EXAMPLE_ESP_TCP_DELAY_INFO CONFIG_TCP_PERF_DELAY_DEBUG

/*AP info and tcp_server info*/
#define EXAMPLE_DEFAULT_SSID CONFIG_TCP_PERF_WIFI_SSID
#define EXAMPLE_DEFAULT_PWD CONFIG_TCP_PERF_WIFI_PASSWORD
#define EXAMPLE_DEFAULT_PORT CONFIG_TCP_PERF_SERVER_PORT
#define EXAMPLE_DEFAULT_PKTSIZE CONFIG_TCP_PERF_PKT_SIZE
#define EXAMPLE_MAX_STA_CONN 1 

#ifdef CONFIG_TCP_PERF_SERVER_IP
#define EXAMPLE_DEFAULT_SERVER_IP CONFIG_TCP_PERF_SERVER_IP
#else
#define EXAMPLE_DEFAULT_SERVER_IP "192.168.4.1"
#endif 
#define EXAMPLE_PACK_BYTE_IS 97 //'a'





#define TCP_SERVER_CLIENT_OPTION FALSE  //true为开启热点并且创建tcp服务器，fasle为连接到指定的路由器并且连接到指定的tcp服务器
#define TAG "XuHongTCP-->" //打印的tag

//以下是softAP热点模式的配置信息
#define SOFT_AP_SSID "XuHongTCP2018"

#define SOFT_AP_PAS "xuhong123456" //如果密码设置为空，则配置的热点是开放的，没有密码的。

#define SOFT_AP_MAX_CONNECT 1 //作为AP热点时候，最大的连接数目


//以下是station模式配置信息,是您家里的路由器的信息

#define GATEWAY_SSID "AliyunOnlyTest"

#define GATEWAY_PAS "aliyun#123456"

#define TCP_SERVER_ADRESS "192.168.1.104" //要连接TCP服务器地址


//统一的端口号，包括TCP客户端或者服务端
#define TCP_PORT 8266














/* FreeRTOS event group to signal when we are connected to wifi*/
extern EventGroupHandle_t tcp_event_group;
#define WIFI_CONNECTED_BIT BIT0

extern int  g_total_data;
extern bool g_rxtx_need_restart;

#if EXAMPLE_ESP_TCP_PERF_TX && EXAMPLE_ESP_TCP_DELAY_INFO
extern int g_total_pack;
extern int g_send_success;
extern int g_send_fail;
extern int g_delay_classify[5];
#endif/*EXAMPLE_ESP_TCP_PERF_TX && EXAMPLE_ESP_TCP_DELAY_INFO*/


//using esp as station
void wifi_init_sta();
//using esp as softap
void wifi_init_softap();

//create a tcp server socket. return ESP_OK:success ESP_FAIL:error
esp_err_t create_tcp_server(bool isCreatServer);
//create a tcp client socket. return ESP_OK:success ESP_FAIL:error
esp_err_t create_tcp_client();

//send data task
void send_data(void *pvParameters);
//receive data task
void recv_data(void *pvParameters);

//close all socket
void close_socket();

//get socket error code. return: error code
int get_socket_error_code(int socket);

//show socket error code. return: error code
int show_socket_error_reason(const char* str, int socket);

//check working socket
int check_working_socket();


#ifdef __cplusplus
}
#endif


#endif /*#ifndef __TCP_PERF_H__*/


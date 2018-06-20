#ifndef __UDP_XUHONG_H__
#define __UDP_XUHONG_H__

#ifdef __cplusplus
extern "C" {
#endif

//全局声明：是否server服务器或者station客户端 ------> true为服务器，false为客户端
#define Server_Station_Option false

#define EXAMPLE_ESP_WIFI_MODE_AP false //TRUE:AP FALSE:STA
#define EXAMPLE_ESP_UDP_MODE_SERVER false //TRUE:server FALSE:client
#define EXAMPLE_ESP_UDP_PERF_TX false //TRUE:send FALSE:receive
#define EXAMPLE_PACK_BYTE_IS 97 //'a'

/*
 * 要连接的路由器名字和密码
 */

//路由器的名字
#define GATEWAY_SSID "AliyunOnlyTest"
//连接的路由器密码
#define GATEWAY_PASSWORD "aliyun#123456"

//数据包大小
#define EXAMPLE_DEFAULT_PKTSIZE 1024

/*
 * 自己作为AP热点时候，配置信息如下
 */
//ssid
#define AP_SSID "XuHong_Esp32"
//密码
#define AP_PAW "xuhong123456"
//最大连接数
#define EXAMPLE_MAX_STA_CONN 1

/*
 * station模式时候，服务器地址配置
 */
//服务器的地址：这里的 255.255.255.255是在局域网发送，不指定某个设备
#define SERVER_IP "255.255.255.255"
//端口号
#define SERVICE_PORT 8265

#define TAG "XuHong_UDP_Demo_ForEsp32 :"

/* FreeRTOS event group to signal when we are connected to WiFi and ready to start UDP test*/
extern EventGroupHandle_t udp_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define UDP_CONNCETED_SUCCESS BIT1

extern int total_data;
extern int success_pack;

//using esp as station
void wifi_init_sta();
//using esp as softap
void wifi_init_softap();

esp_err_t create_udp_server();
esp_err_t create_udp_client();

//send or recv data task
void send_recv_data(void *pvParameters);

//get socket error code. return: error code
int get_socket_error_code(int socket);

//show socket error code. return: error code
int show_socket_error_reason(int socket);

//check connected socket. return: error code
int check_connected_socket();

int send_Buff_with_UDP(char *databuff, int length);

//close all socket
void close_socket();

#ifdef __cplusplus
}
#endif

#endif


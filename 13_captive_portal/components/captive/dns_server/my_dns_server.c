#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "cJSON.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include "mbedtls/md5.h"
#include "webserver.h"

static const char *TAG = "DNS_SERVER";

static int create_udp_socket(int port)
{
    struct sockaddr_in saddr = { 0 };
    int sock = -1;
    int err = 0;

    sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0) {
        ESP_LOGE(TAG,"Failed to create socket. Error %d", errno);
        return -1;
    }

    // Bind the socket to any address
    saddr.sin_family = PF_INET;
    saddr.sin_port = htons(port);
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);
    err = bind(sock, (struct sockaddr *)&saddr, sizeof(struct sockaddr_in));
    if (err < 0) {
        ESP_LOGE(TAG,"Failed to bind socket. Error %d", errno);
        goto err;
    }

    // All set, socket is configured for sending and receiving
    return sock;

err:
    close(sock);
    return -1;
}

static void my_dns_server(void *pvParameters)
{
/* Wait for all the IPs we care about to be set
    */
    uint8_t data[128];
    int len = 0;
    struct sockaddr_in client = { 0 };
    socklen_t  client_len=sizeof(struct sockaddr_in); 
    uint32_t i = 0;

    ESP_LOGI(TAG,"DNS server start ...");

    int sock = create_udp_socket(53);

    if (sock < 0) {
        ESP_LOGE(TAG,"Failed to create IPv4 multicast socket");
    }

    while(1)
    {

        len=recvfrom(sock,data,100,0,(struct sockaddr *)&client,&client_len); //阻塞式

        if((len < 0) || ( len > 100))
        {
            ESP_LOGE(TAG,"recvfrom error\n");
            continue;
        }

        printf("DNS request:");
        for(i = 0x4; i< len;i++)
        {
            if((data[i] >= 'a' && data[i] <= 'z') || (data[i] >= 'A' && data[i] <= 'Z') ||(data[i] >= '0' && data[i] <= '9'))
                printf("%c",data[i]);
            else
                printf("_");

        }
        printf("\r\n");
        //printf("%d\r\n",esp_get_free_heap_size()); //打印系统可用内存

        //过滤掉一些乱糟糟的域名
        if( strstr((const char *)data+0xc,"taobao")||
            strstr((const char *)data+0xc,"qq")    || 
            strstr((const char *)data+0xc,"sogou") ||
            strstr((const char *)data+0xc,"amap")  ||
            strstr((const char *)data+0xc,"alipay")||
            strstr((const char *)data+0xc,"youku") ||
            strstr((const char *)data+0xc,"iqiyi") ||
            strstr((const char *)data+0xc,"baidu"))
        {
            continue;
        }

        data[2] |= 0x80;
        data[3] |= 0x80;
        data[7] =1;

        data[len++] =0xc0;
        data[len++] =0x0c;

        data[len++] =0x00;
        data[len++] =0x01;
        data[len++] =0x00;
        data[len++] =0x01;

        data[len++] =0x00;
        data[len++] =0x00;
        data[len++] =0x00;
        data[len++] =0x0A;

        data[len++] =0x00;
        data[len++] =0x04;

        data[len++] =192;
        data[len++] =168;
        data[len++] =4;
        data[len++] =1;

        /*打印客户端地址和端口号*/
        // inet_ntop(AF_INET,&client.sin_addr,(char *)data,sizeof(data));
        // printf("client IP is %s, port is %d\n",data,ntohs(client.sin_port));

        sendto(sock,data,len,0,(struct sockaddr*)&client,client_len);

        vTaskDelay(10);
    }

    ESP_LOGE(TAG,"DNS　server stop ...");
    shutdown(sock, 0);
    close(sock);
    vTaskDelete(NULL);
}

void dns_server_start()
{
    xTaskCreate(&my_dns_server, "dns_task", 2048, NULL, 5, NULL);
}

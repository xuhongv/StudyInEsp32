/*
 * @Description: wechat xBlufi to net
 * @Author: 徐宏 xuhong
 * @Date: 2019-10-03 16:36:21
 * @LastEditTime : 2019-12-20 11:15:47
 * @LastEditors  : Please set LastEditors
 */
#ifndef X_xBlufi_H_
#define X_xBlufi_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "string.h"

typedef enum
{
     xBlufi_MSG_TYPE_SATRT = 0,            // 开始
     xBlufi_MSG_TYPE_GET_BSSID,            // 成功获取路由器mac地址
     xBlufi_MSG_TYPE_GET_SSID,             // 成功获取路由器名字
     xBlufi_MSG_TYPE_GET_PASSWORD,         // 成功获取路由器密码
     xBlufi_MSG_TYPE_REQ_CONNECT_TO_AP,    //客户端要求连接热点
     xBlufi_MSG_TYPE_REQ_DISCONNECT_TO_AP, //客户端要求断开连接热点

     xBlufi_MSG_TYPE_RECIEVE_CUSTON_DATA,      //  接收到自定义数据
     xBlufi_MSG_TYPE_SEND_ACK_TO_WEICHAT_OVER, // 配网成功后，设备端发送配网成功的ack到微信端
     xBlufi_MSG_TYPE_CONNECTED_BLE,            // 与蓝牙客户端成连接
     xBlufi_MSG_TYPE_DIS_CONNECT_BLE,          // 与蓝牙客户端断开连接

     xBlufi_MSG_TYPE_NOTIFY_CONNECT_ACK, // 通知蓝牙客户端已成功连接路由器

} xBlufiMsgType;

typedef struct
{
     char data[100];
     uint16_t dataLen;

     xBlufiMsgType type;
} xBlufiMsg;

/**
     * @description: 开始 esp32 配置
     * @param {type} 
     * @return: 
     */
esp_err_t xBlufi_start();

/**
 * @description: 
 * @param {type} 
 * @return: 
 */
void XBlufi_notify_got_ip(void);

/**
 * @description: 通知连接成功
 * @param {type} 需要路由器的ssid
 * @return: 
 */
void XBlufi_notify_connected(uint8_t *bssid);

/**
 * @description: 通知连接失败
 * @param {type} 
 * @return: 
 */
void XBlufi_notify_connect_fail(void);

/**
     * @description: 结束 esp32 微信配网配置和近场发现
     * @param {type} 
     * @return: 
     */
esp_err_t xBlufi_stop_all();
/**
     * @description: 主动监听事件
     * @param {type} 
     * @return: 
     */
esp_err_t xBlufiReceiveMsg(xBlufiMsg *msg);

/**
 * @description: 
 * @param {type} 
 * @return: 
 */
esp_err_t XBlufi_notify_send_custom_data(uint8_t *data, uint32_t len);

/**
 * @description: 
 * @param {type} 
 * @return: 
 */
void XBlufi_notify_set_blufi_name(char *name, uint8_t len);

#endif /* X_xBlufi_H_ */

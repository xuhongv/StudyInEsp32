# take_pic_http_to_cloud

# 目录

- [1.功能描述](#Funtion)  
- [2.准备](#hardwareprepare)  
- [3.服务器准备](#clouds)  
- [4.嵌入式代码详解](#device)  
- [5.推荐开源好玩DIY的一览表](#other)  

## <span id = "Funtion">一、功能描述</span>

本文基于 linux 环境，通过安信可 ESP32-Cam 开发板 SDK C语言编程二次开发，本地拍照图片上传到指定的服务器。

## <span id = "Introduction">二、准备</span>

### 硬件

- 安信可 ESP32-Cam 开发板：https://item.taobao.com/item.htm?id=573698917181

- TTL-USB 调试工具（推荐使用这个，保证足够的电压电流）：https://item.taobao.com/item.htm?id=565546260974

### 软件

- 环境搭建：[Linux环境搭建 /relese/v3.3.2 分支](https://docs.espressif.com/projects/esp-idf/zh_CN/v3.3.2/get-started/linux-setup.html)
- esp-idf commitId: `b4c0751692a18db098a9a6139b2ab5a789d39167`
- 工具链设置：下载 toolchain，博主使用的版本是：```gcc version 5.2.0 (crosstool-NG crosstool-ng-1.22.0-97-gc752ad5)```
- Python版本：`Python 2.7.17`

## <span id = "clouds">三、服务器准备</span>

本代码以HTTP 协议POST提交 ，数据流形式把图片上传到服务器。其格式如下：

```c
POST /index.php HTTP/1.1
Host: www.domain.com
Content-Type: image/jpeg
Content-Length: 12540

"<file contents here>"
```

因此，服务器要以数据流接收，以 `PHP`语言为例：

```php
//接受数据流
$stream = file_get_contents('php://input');
//数据流转化为png格式，并保存在指定的位置
$len = file_put_contents('/www/wwwroot/static/' . time() . ".png", $stream); 
```

## <span id = "device">四、嵌入式代码详解</span>

文件目录说明：

```
├─1_take_pic_http_to_cloud 
│ ├─components 摄像头驱动代码组件
│ ├─main 用户程序
│ │ ├─app_wifi.c/h 连接路由器的逻辑实现
│ │ ├─main.c 主文件程序入口
```

主文件逻辑代码协议详解：

```c
static void http_test_task(void *pvParameters)
{

    //等待Wi-Fi连接成功
    app_wifi_wait_connected();

    ESP_LOGI(TAG, "Http Start");
    size_t _jpg_buf_len;
    uint8_t *_jpg_buf;
    camera_fb_t *pic = NULL;
    esp_err_t res = ESP_OK;
    size_t fb_len = 0;

    //配置服务器相关信息
    esp_http_client_config_t config = {
        .url = "http://www.domain.com/index.php",
        .method = HTTP_METHOD_POST,
        .event_handler = _http_event_handler,
        .buffer_size = 4096,
        .timeout_ms = 10000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);

    //开始拍照
    pic = esp_camera_fb_get();

    if (!pic)
    {
        ESP_LOGE(TAG, "Camera capture failed");
    }
    else
    {
        //拍照成功，获取其大小、尺寸等信息
        ESP_LOGI(TAG, "Camera capture OK , Its size was: %zu bytes", pic->len);
        ESP_LOGI(TAG, "Camera capture OK , Its width was: %d", pic->width);
        ESP_LOGI(TAG, "Camera capture OK , Its height was: %d ", pic->height);

        //设置HTTP请求头为image/jpg表示图片类型
        res = esp_http_client_set_header(client, "Content-Type", "image/jpg");
        if (res != ESP_OK)
        {
            ESP_LOGE(TAG, "esp_http_client_set_header result code : [%s]", esp_err_to_name(res));
            goto end;
        }

        //把图片放在body里面
        res = esp_http_client_set_post_field(client, (const char *)pic->buf, pic->len);
        if (res != ESP_OK)
        {
            ESP_LOGE(TAG, "esp_http_client_set_post_field result  code : [%s]", esp_err_to_name(res));
            goto end;
        }

        vTaskDelay(1000 / portTICK_RATE_MS);

        //开始执行请求服务器
        res = esp_http_client_perform(client);

        //判断是否请求成功
        if (res == ESP_OK)
        {
         ESP_LOGI(TAG, "HTTPS Status = %d, content_length = %d", 
                  esp_http_client_get_status_code(client), esp_http_client_get_content_length(client));
        }
        else
        {
            ESP_LOGE(TAG, "perform http request %s", esp_err_to_name(res));
        }
    }

end:
{
    esp_camera_fb_return(pic);
    esp_http_client_cleanup(client);
    free(buffer);
}

    ESP_LOGI(TAG, "Http End");
    vTaskDelete(NULL);
}
```

## <span id = "device">五、推荐开源好玩DIY的一览表

| 开源项目                                                 | 地址                                                        | 开源时间   |
| -------------------------------------------------------- | ----------------------------------------------------------- | ---------- |
| ESP32-Cam摄像头拍照上传到私有服务器                      |                                                             | 2020.12.30 |
| 微信小程序连接mqtt服务器，控制esp8266智能硬件            | https://github.com/xuhongv/WeChatMiniEsp8266                | 2018.11    |
| 微信公众号airkiss配网以及近场发现在esp8266的实现         | https://github.com/xuhongv/xLibEsp8266Rtos3.1AirKiss        | 2019.3     |
| 微信公众号airkiss配网以及近场发现在esp32/esp32S2的实现   | https://github.com/xuhongv/xLibEsp32IdfAirKiss              | 2019.9     |
| 微信小程序控制esp8266实现七彩效果项目源码                | https://github.com/xuhongv/WCMiniColorSetForEsp8266         | 2019.9     |
| 微信小程序蓝牙配网blufi实现在esp32源码                   | https://github.com/xuhongv/BlufiEsp32WeChat                 | 2019.11    |
| 微信小程序蓝牙ble控制esp32七彩灯效果                     | https://blog.csdn.net/xh870189248/article/details/101849759 | 2019.10    |
| 可商用的事件分发的微信小程序mqtt断线重连框架             | https://blog.csdn.net/xh870189248/article/details/88718302  | 2019.2     |
| 微信小程序以 websocket 连接阿里云IOT物联网平台mqtt服务器 | https://blog.csdn.net/xh870189248/article/details/91490697  | 2019.6     |
| 微信公众号网页实现连接mqtt服务器                         | https://blog.csdn.net/xh870189248/article/details/100738444 | 2019.9     |
| 微信小程序AP配网Wi-Fi ESP32-S2模块                       | https://github.com/xuhongv/WeChatMiniAP2Net                 | 2020.9.21  |
| 云云对接方案服务器源码 xClouds-php PHP TP5开发框架       | https://github.com/xuhongv/xClouds-php                      | 2020.8.4   |
| 微信小程序端自定义view七彩采集颜色圆环控件               | https://github.com/xuhongv/WCMiniColorPicker                | 2019.12.04 |
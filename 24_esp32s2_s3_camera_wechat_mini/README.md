# 安信可ESP32S3一键拍照上传到阿里云对象存储OSS

# 目录

- [1.功能描述](#product)  
- [2.准备](#ready)  
- [3.服务器准备](#clouds)  
- [4.嵌入式代码详解](#device)  
- [5.推荐开源好玩DIY的一览表](#show)  

## <span id = "Funtion">一、功能描述</span>



![在这里插入图片描述](https://img-blog.csdnimg.cn/e197ab9bb18848a28b041499f1972678.jpg?x-oss-process=image/watermark,type_ZHJvaWRzYW5zZmFsbGJhY2s,shadow_50,text_Q1NETiBA5Y2K6aKX5b-D6ISP,size_20,color_FFFFFF,t_70,g_se,x_16#pic_center)
# 前言
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;第一批拿到了安信可 ```ESP32-S3```模组的样品，今天给大家分享下这个模组的规格，此模组精准聚焦 AIoT 市场，响应市场对 AI 算法的技术需求，那么此文章介绍的是```ESP32-S3```模组上驱动摄像头 ```OV2640```，后续将继续更新此系列博文。

----------------------

![在这里插入图片描述](https://img-blog.csdnimg.cn/aad238041d18499badc6992774c22673.jpg?x-oss-process=image/watermark,type_ZHJvaWRzYW5zZmFsbGJhY2s,shadow_50,text_Q1NETiBA5Y2K6aKX5b-D6ISP,size_20,color_FFFFFF,t_70,g_se,x_16#pic_center)
# <span id = "product">一、 ESP32-S3 产品特性
&nbsp;&nbsp;&nbsp;拿到了安信可的ESP32-S3样品，当然得查看其规格啦。
## CPU 和存储
 
• **Xtensa® 32 位 LX7** 双核处理器，主频高达 240 MHz 
 
• 128 位数据总线位宽，支持 SIMD 指令 
 
• 384 KB ROM
 
• 512 KB SRAM
 
• 16 KB RTC SRAM 
 
• SPI、Dual SPI、Quad SPI、Octal SPI、QPI、OPI 接口外接多个 flash 和片外 RAM

## 外设接口和传感器

• 45 × GPIO 口 • 数字接口：
 
• 4 × SPI 
 
•  1 × LCD 接口（8 位 ~16 位并行 RGB, I8080, MOTO6800）, 支持 RGB565, YUV422, YUV420, YUV411 之间互相转换
 
•  1 × DVP 8 位 ~16 位摄像头接口 
 
•  3 × UART
 
•  2 × I2C
 
•  2 × I2S
 
•  1 × RMT (TX/RX) 
 
•  1 × 脉冲计数器 
 
•   LED PWM 控制器，多达 8 个通道 
 
•  1 × 全速 USB OTG 
 
•  1 × USB Serial/JTAG 控制器 
 
•  2 × MCPWM 
 
•  1 × SDIO 主机接口，具有 2 个卡槽 
 
•  DMA 控制器，5 个接收通道和 5 个发送通道 
 
•  1 × TWAI® 控制器（兼容 ISO11898-1） 
 
•  2 × 12 位 SAR ADC，多达 20 个通道 
 
 
•  1 × 温度传感器 – 14 × 电容式传感 GPIO • 定时器： 
 
•  4 × 54 位通用定时器 
 
 •  1 × 52 位系统定时器 
 
 •    3 × 看门狗定时器 

## 功耗特性

&nbsp;&nbsp;&nbsp;ESP32-S3 采用了先进的电源管理技术，可以在不同的功耗模式之间切换。ESP32-S3 支持的功耗模式有：

• **Active 模式**：CPU 和芯片射频处于工作状态。芯片可以接收、发射和侦听信号。 
 
• **Modem-sleep 模式**：CPU 可运行，时钟频率可配置。Wi-Fi 基带和射频关闭，但 Wi-Fi 可保持连接。 
 
• **Light-sleep 模式**：CPU 暂停运行。RTC 外设以及 ULP 协处理器可被定时器周期性唤醒运行。任何唤醒事 件（MAC、主机、RTC 定时器或外部中断）都会唤醒芯片。Wi-Fi 可保持连接。 
 
• **Deep-sleep 模式**：CPU 和大部分外设都会掉电，只有 RTC 存储器和 RTC 外设处于工作状态。Wi-Fi 连接 数据存储在 RTC 中。ULP 协处理器可以工作。
 
• **Hibernation 模式**：内置快速 RC 振荡器时钟和 ULP 协处理器均被禁用。只有 1 个位于低速时钟上的 RTC 时钟定时器和某些 RTC GPIO 在工作。RTC 时钟定时器或 RTC GPIO 可以将芯片从 Hibernation 模式中唤 醒。 设备在不同的功耗模式下有不同的电流消耗，详情请见下面：	

| 功耗模式 | 描述 | 典型值 | 单位 |
| --- | ----| ----| ---|
| Light-sleep | CPU 暂停运行。|  240|  µA |
| Deep-sleep | RTC 存储器和 RTC 外设处于工作状态 | 8|  µA |
| Hibernation | RTC 存储器处于工作状态，RTC 外设处于关闭状态 | 7 |  µA|
| Power off | CHIP_PU 管脚拉低，芯片处于关闭状态 | 1 | µA|

----------

# <span id = "ready">二、 准备工作
### 源码下载

模组代码：https://github.com/xuhongv/StudyInEsp32/tree/master/24_esp32s2_s3_camera_wechat_mini

微信小程序代码：https://github.com/xuhongv/RemoteWeChatForESP

服务器代码：见本文章服务器代码介绍章节。

### 硬件

- 安信可 ESP32-S3 模组。

- TTL-USB 调试工具（推荐使用 CP2102串口芯片的 ，保证足够的电压电流）。

### 软件

- 环境搭建：[ Linux环境搭建 master 分支 ](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32/get-started/index.html)
- Python版本：`Python 3.8`

### 接线

| 摄像头引脚 | 模组引脚|
|-----|-----|
|pin_pwdn | GPIO_NUM_35|
|pin_reset | GPIO_NUM_36|
|pin_xclk | GPIO_NUM_15|
|pin_sscb_sda | GPIO_NUM_4|
|pin_sscb_scl | GPIO_NUM_5|
|pin_d7 | GPIO_NUM_16 |
|pin_d6 | GPIO_NUM_17 |
|pin_d5 | GPIO_NUM_18|
|pin_d4 | GPIO_NUM_12|
|pin_d3 | GPIO_NUM_11|
|pin_d2 | GPIO_NUM_10|
|pin_d1 | GPIO_NUM_9|
|pin_d0 | GPIO_NUM_8 |
|pin_vsync | GPIO_NUM_6|
|pin_href | GPIO_NUM_7 |
|pin_pclk | GPIO_NUM_13 |

-----------------------
# <span id = "chat">三、通讯协议和原理

![在这里插入图片描述](https://img-blog.csdnimg.cn/cfaf31d66f544dc68855365332852f61.png?x-oss-process=image/watermark,type_ZHJvaWRzYW5zZmFsbGJhY2s,shadow_50,text_Q1NETiBA5Y2K6aKX5b-D6ISP,size_20,color_FFFFFF,t_70,g_se,x_16#pic_center)
## 3.1 模组拍照上传服务器
协议： HTTP
```
POST /index.php HTTP/1.1
Host: www.domain.com
Content-Type: image/jpeg
Name-Pic: name
Content-Length: 12540


"<file contents here>"
```
-----------------------
## 3.2 小程序下发指令给模组拍照给服务器
协议：MQTT
Topic：**/light/deviceIn**
payload：
```
{
    "action" : "0" ,
    "name" : "pic-name"
}
```
--------------------
## 3.3 模组把上传服务器结果回调给小程序
协议：MQTT
Topic：**/light/deviceOut**
payload：
```
{
  "url" : "base64(url)" 
}
```


url 是拍照之后的可外网访问的图片地址，使用 base64 加密。

----------------------
# <span id = "clouds">四、服务器准备

服务器我这边使用的是基于PHP语言编程的TP5框架，而MQTT服务器使用的是百度天工

## 3.1 接收图片处理

本代码以HTTP 协议POST提交 ，二进制形式把图片上传到服务器。其格式如下：

```c
POST /index.php HTTP/1.1
Host: www.domain.com
Content-Type: image/jpeg
Name-Pic: name
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

## 3.2 接收图片处理
```php
    public function upload()
    {
        $getPostDataArry = apache_request_headers();

        try {
            $name = $getPostDataArry['Name-Pic'];
        } catch (Exception $e) {
            if (empty($name)) {
                $response_data['code'] = 1;
                $response_data['des'] = ' Name-Pic is not exist';
                return json($response_data);
            }
        }
        //接受数据流
        $stream = file_get_contents('php://input');
        //数据流转化为png格式，并保存在指定的位置
        $len = file_put_contents('/www/wwwroot/www.xuhongv.com/public/icon/' . $name . ".png", $stream);
        $response_data = [];
        if ($len) {
            $accessKeyId = "LTAI4Fg*********YJBLPoTo5";
            $accessKeySecret = "OT7s9vkQd*******4p8KQ31qoTIL4";
            $endpoint = "we****ong.oss-cn-hongkong.aliyuncs.com";
            $bucket = 'web****hong';
            try {
                $ossClient = new OssClient($accessKeyId, $accessKeySecret, $endpoint, true);
                //判断存储空间是否存在
                $isExist = $ossClient->doesBucketExist($bucket);
                if (!$isExist) {
                    $response_data['code'] = 1;
                    $response_data['des'] = $endpoint . ' is not exist';
                    return json($response_data);
                }
                $options = array();
                //oss 存储文件路径
                $object = "images/" . $name . ".png";
                $ossClient->uploadFile($bucket, $object, '/www/wwwroot/www.xuhongv.com/public/icon/' . $name . ".png", $options);
            } catch (OssException $e) {
                $response_data['code'] = 1;
                $response_data['des'] = $e->getMessage();
                return json($response_data);
            }
            $response_data['code'] = 0;
            $response_data['des'] = 'uploadup ok';
            $response_data['url'] = base64_encode("https://" . $endpoint . '/' . $object);
            return json($response_data);
        }
        $response_data['code'] = 1;
        $response_data['des'] = "error , the upload file length is" . $len;
        return json($response_data);
    }
}
```
## 3.3 服务器业务逻辑

 1. 接受到S3模组发过来的图片，存储到本地服务器某个文件夹里面。
 2. 同时把这个图片，发送到阿里云对象存储，返回一个可远程访问图片的地址给模组。

----------------------
# <span id = "device">五、嵌入式开发准备
配置服务器信息，然后拍照上传。
```
    //配置服务器相关信息
    esp_http_client_config_t config = {
        .url = "http://domain.xuhongv.com/api/upload",
        .method = HTTP_METHOD_POST,
        .event_handler = _http_event_handler,
        .buffer_size = 4096,
        .user_data = local_response_buffer, 
        .buffer_size_tx = 4096 * 5,
        .timeout_ms = 10000
    };

    //开始拍照
    pic = esp_camera_fb_get();

    if (!pic)
    {
        ESP_LOGE(TAG, "Camera capture failed");
        goto end;
    }
    else
    {
        esp_camera_fb_return(pic);

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

            esp_http_client_handle_t esp_client = esp_http_client_init(&config);

            //设置HTTP请求头为image/jpg表示图片类型
            esp_http_client_set_header(esp_client, "Content-Type", "image/jpg");

            //设置图片名字
            esp_http_client_set_header(esp_client, "Name-Pic", &(test_task_param->pic_name_p));

            //把图片放在body里面
            esp_http_client_set_post_field(esp_client, (const char *)pic->buf, pic->len);

            //开始执行请求服务器
            res = esp_http_client_perform(esp_client);

            //判断是否请求成功
            if (res == ESP_OK)
            {
                ESP_LOGI(TAG, "HTTPS Status = %d", esp_http_client_get_status_code(esp_client));
            }
            else
            {
                ESP_LOGE(TAG, "perform http request %s", esp_err_to_name(res));
            }
            ESP_ERROR_CHECK(esp_http_client_cleanup(esp_client));
        }
    }
```



# <span id = "mini">六、微信小程序开发准备

配置服务器信息，然后拍照上传。

```
    data: {
    host: 'a0je61b5.mqtt.iot.gz.baidubce.com',
    subTopic: '/light/deviceOut',
    pubTopic: '/light/deviceIn',
    msg: 'Hello! I am from WeChat miniprogram',
    //默认显示的图片
    img_url: 'https://docs.ai-thinker.com/_media/o1cn01faxszt1ls4mym2mkb_2922621297.png',
    mqttOptions: {
      protocolVersion: 4, //MQTT连接协议版本
      clientId: 'DeviceId-jviujtntjy',
      clean: true,
      password: 'sM2hrD6kcRFlmIgR2F',
      username: 'a0je61a/wechat',
      reconnectPeriod: 1000, // 1000毫秒，两次重新连接之间的间隔
      connectTimeout: 30 * 1000, // 1000毫秒，两次重新连接之间的间隔
      resubscribe: true // 如果连接断开并重新连接，则会再次自动订阅已订阅的主题（默认true）
     }
    }
    
    let that = this

    client = mqtt.connect(`wxs://${this.data.host}/mqtt`, this.data.mqttOptions)

    client.on('reconnect', (error) => {
      console.log('Reconnecting...')
    })

    client.on('error', (error) => {
      console.log('连接失败:', error)
    })

    client.on('connect', () => {
      wx.showToast({
        title: '连接成功'
      })

      client.subscribe(this.data.subTopic, (err) => {
      })
      client.on('message', (topic, payload) => {
        let object = JSON.parse(payload)
        //base64解码url地址
        let url = that.base64_decode(object.url)
        console.log(url)
        that.setData({
          img_url: url
        })
        wx.showModal({
          content: `收到 Topic: ${topic}, Payload:  ${url}`,
          showCancel: false,
        });
      })
    })
```


## <span id = "show">七、推荐开源好玩DIY的一览表

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

## 八、讨论交流

<table>
  <tbody>
    <tr >
      <td align="center" valign="middle" style="border-style:none">
       <img class="QR-img" height="260" width="260" src="https://aithinker-static.oss-cn-shenzhen.aliyuncs.com/bbs/important/qq_group.png">
        <p style="font-size:12px;">QQ群号：434878850</p>
      </td>
      <td align="center" valign="middle" style="border-style:none">
        <img class="QR-img" height="260" width="260" src="https://aithinker-static.oss-cn-shenzhen.aliyuncs.com/bbs/important/wechat_account.jpg">
        <p style="font-size:12px;">本人微信公众号：徐宏blog</p>
      </td>
      <td align="center" valign="middle" style="border-style:none">
        <img class="QR-img" height="260" width="260" src="https://aithinker-static.oss-cn-shenzhen.aliyuncs.com/bbs/important/wechat_me.jpg">
        <p style="font-size:12px;">私人工作微信，添加标明来意</p>
      </td>
    </tr>
  </tbody>
</table>

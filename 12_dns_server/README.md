# ESP32 dns_server

说明：将 ESP32 当做一个 dns_server and web_server，通過在客戶端登陸網頁域名：www.espressif.com 登陸網頁，獲得特定返回。
## Quick start

### 必須

- 你已经安装好 ESP-IDF 和工具链。
- 你已经有一个手機或者電腦客戶端。
- 將 ESP32 設置為 softAP 模式。

### 步骤

- 使用数据线将开发板连接到你的系统中，让系统能够识别到你的板子（Windows 是`COM\*`, Linux 是`/dev/ttyUSB\*`）。
- 进入`sta`所在目录。
- 执行命名`make menuconfig`进行配置。
  - 对热点的 SSID 和密码进行配置。依次进入配置选项`Demo Configuration  --->`，然后在`WiFi SSID`和`WiFi Password`中填写你的 SSID 和密码。然后退出配置菜单，保存配置。
  - 对串口进行配置。
- 执行命令`make`进行编译
- 执行命令`make flash monitor`将编译生成的镜像烧写到 ESP32 开发板上面，并查看串口输出。
- 在客戶端打開瀏覽器，輸入 www.espressif.com ，獲得網頁返回。

### 现象

![](http://i.imgur.com/ED5kuKJ.jpg)


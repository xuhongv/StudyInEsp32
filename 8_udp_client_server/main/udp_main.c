#include <errno.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "XuHongUdp.h"

static void udp_conn(void *pvParameters) {

	ESP_LOGI(TAG, "task udp_conn start... \n\r");
	//等待是否已经成功连接到路由器的标志位
	xEventGroupWaitBits(udp_event_group, WIFI_CONNECTED_BIT, false, true,
			portMAX_DELAY);

	ESP_LOGI(TAG,
			"esp32 is ready !!! create udp client or connect servece after 5s... \n\r");
	vTaskDelay(5000 / portTICK_RATE_MS);

//创建客户端并且检查是否创建成功
#if Server_Station_Option
	ESP_LOGI(TAG, "Now Let us create udp server ... \n\r");
	if (create_udp_server() == ESP_FAIL) {
		ESP_LOGI(TAG, " server create socket error , stop !!! \n\r");
		vTaskDelete(NULL);
	} else {
		ESP_LOGI(TAG, "server create socket Succeed  !!! \n\r");
	}
#else
	ESP_LOGI(TAG, "Now Let us create udp client ... \n\r");
	if (create_udp_client() == ESP_FAIL) {
		ESP_LOGI(TAG, "client create socket error , stop !!! \n\r");
		vTaskDelete(NULL);
	} else {
		ESP_LOGI(TAG, "client create socket Succeed  !!! \n\r");
	}
#endif

	//创建一个发送和接收数据的任务
	TaskHandle_t tx_rx_task;
	xTaskCreate(&send_recv_data, "send_recv_data", 4096, NULL, 4, &tx_rx_task);

	//等待 UDP连接成功标志位
	xEventGroupWaitBits(udp_event_group, UDP_CONNCETED_SUCCESS, false, true,
			portMAX_DELAY);

	int bps;
	char sendBuff[1024] = "hello xuhong,I am from Esp32 ...";

	while (1) {

		total_data = 0;

		vTaskDelay(3000 / portTICK_RATE_MS);
		//时隔三秒发送一次数据
      #if !Server_Station_Option
		send_Buff_with_UDP(sendBuff, 1024);
      #endif
		bps = total_data / 3;

		if (total_data <= 0) {
			int err_ret = check_connected_socket();
			if (err_ret == -1) {
				ESP_LOGW(TAG,
						"udp send & recv stop !!! will close socket ... \n\r");
				close_socket();
				break;
			}
		}

		ESP_LOGI(TAG, "udp recv %d byte per sec! total pack: %d \n\r", bps,
				success_pack);

	}

	vTaskDelete(tx_rx_task);
	vTaskDelete(NULL);
}

void app_main(void) {

	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}

#if Server_Station_Option
	wifi_init_softap();
	ESP_LOGI(TAG, "UDP AP Server Demo ...\n\r");
#else
	wifi_init_sta();
	ESP_LOGI(TAG, "UDP Station Client Demo ...\n\r");
#endif

	xTaskCreate(&udp_conn, "udp_conn", 4096, NULL, 5, NULL);
}

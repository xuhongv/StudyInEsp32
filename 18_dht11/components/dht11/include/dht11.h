/*
 * @Author: xuhongv
 * @Date: 2020-06-06 10:43:04
 * @LastEditTime: 2020-06-10 10:48:30
 * @LastEditors: Please set LastEditors
 * @Description: origin from https://github.com/hamsternz/thingspeak-esp32-dht11
 * @FilePath: https://github.com/xuhongv
 */

#ifndef DHT11_H_
#define DHT11_H_

#include <driver/rmt.h>
#include <soc/rmt_reg.h>
#include "driver/gpio.h" 

// 定义的GPIO口
#define DHT11_GPIO 4

/**
 * @description: 初始化
 * @param {type} 
 * @return: 
 */
void dht11_init();

/**
 * @description: 获取温湿度
 * @param {type} temperature温度，humidity湿度
 * @return: 
 */
int dht11_start_get(int *temperature, int *humidity);

#endif
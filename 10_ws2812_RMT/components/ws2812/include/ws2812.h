#ifndef WS2812_DRIVER_H
#define WS2812_DRIVER_H

#include <stdint.h>

typedef union {
  struct __attribute__ ((packed)) {
    uint8_t r, g, b;
  };
  uint32_t num;
} rgbValue;


/**
 * @description: 转换rgb
 * @param {type} 
 * @return: 
 */
inline rgbValue getRGBValue(uint8_t r, uint8_t g, uint8_t b)
{
  rgbValue v;
  v.r = r;
  v.g = g;
  v.b = b;
  return v;
}


/**
 * @description: 初始化io口
 * @param {type} gpio: 驱动ws2812的gpio口，比如 18
 *               counts: 灯珠个数
 * @return: 
 */
void ws2812_init(int gpio,unsigned int counts);

/**
 * @description: 设置颜色
 * @param {type} 此发送格式为rgb顺序
 * @return: 
 */
void ws2812_setColor(uint8_t r, uint8_t g, uint8_t b);

/**
 * @description: 设置颜色
 * @param {type} 此发送格式为grb顺序
 * @return: 
 */
void ws2812_setColor_grb(uint8_t g, uint8_t r, uint8_t b);


/**
 * @description: 设置颜色数组，具体见demo
 * @param {type} 
 * @return: 
 */
void ws2812_setColors(rgbValue *valueArray);

/**
 * @description: 设置灯珠个数
 * @param {type} 
 * @return: 
 */
void ws2812_setCounts(unsigned int counts);


#endif 

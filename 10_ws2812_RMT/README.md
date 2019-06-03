WS2812 driver and rainbow demo by Chris Osborn <fozztexx@fozztexx.com>

This is a driver for the WS2812 RGB LEDs which uses the RMT peripheral
on the ESP32. It uses interrupts from the RMT peripheral to keep the
buffer filled until all the bytes have been sent to the LED array.

For more information see http://insentricity.com/a.cl/268



 利用GPIO中断搞一个按键的短按和长按，注意demo的是GPIO14短脚！
 
 
### Example output

Here is an example of smartconfig console output.
```
entry 0x40078fb4
[0;32mI (30) boot: ESP-IDF v3.0-12-gdc304fb3-dirty 2nd stage bootloader[0m
[0;32mI (30) boot: compile time 15:10:24[0m
[0;32mI (30) boot: Enabling RNG early entropy source...[0m
[0;32mI (36) boot: SPI Speed      : 40MHz[0m
[0;32mI (40) boot: SPI Mode       : DIO[0m
[0;32mI (44) boot: SPI Flash Size : 4MB[0m
[0;32mI (48) boot: Partition Table:[0m
[0;32mI (52) boot: ## Label            Usage          Type ST Offset   Length[0m
[0;32mI (59) boot:  0 nvs              WiFi data        01 02 00009000 00006000[0m
[0;32mI (67) boot:  1 phy_init         RF data          01 01 0000f000 00001000[0m
[0;32mI (74) boot:  2 factory          factory app      00 00 00010000 00100000[0m
[0;32mI (82) boot: End of partition table[0m
[0;32mI (86) esp_image: segment 0: paddr=0x00010020 vaddr=0x3f400020 size=0x06aa0 ( 27296) map[0m
[0;32mI (104) esp_image: segment 1: paddr=0x00016ac8 vaddr=0x3ffb0000 size=0x021e8 (  8680) load[0m
[0;32mI (108) esp_image: segment 2: paddr=0x00018cb8 vaddr=0x40080000 size=0x00400 (  1024) load[0m
[0;32mI (113) esp_image: segment 3: paddr=0x000190c0 vaddr=0x40080400 size=0x06f50 ( 28496) load[0m
[0;32mI (133) esp_image: segment 4: paddr=0x00020018 vaddr=0x400d0018 size=0x1404c ( 81996) map[0m
[0;32mI (162) esp_image: segment 5: paddr=0x0003406c vaddr=0x40087350 size=0x0168c (  5772) load[0m
[0;32mI (165) esp_image: segment 6: paddr=0x00035700 vaddr=0x400c0000 size=0x00000 (     0) load[0m
[0;32mI (174) boot: Loaded app from partition at offset 0x10000[0m
[0;32mI (175) boot: Disabling RNG early entropy source...[0m
[0;32mI (181) cpu_start: Pro cpu up.[0m
[0;32mI (184) cpu_start: Starting app cpu, entry point is 0x40080e0c[0m
[0;32mI (0) cpu_start: App cpu up.[0m
[0;32mI (195) heap_init: Initializing. RAM available for dynamic allocation:[0m
[0;32mI (202) heap_init: At 3FFAE6E0 len 00001920 (6 KiB): DRAM[0m
[0;32mI (208) heap_init: At 3FFB29F8 len 0002D608 (181 KiB): DRAM[0m
[0;32mI (214) heap_init: At 3FFE0440 len 00003BC0 (14 KiB): D/IRAM[0m
[0;32mI (220) heap_init: At 3FFE4350 len 0001BCB0 (111 KiB): D/IRAM[0m
[0;32mI (227) heap_init: At 400889DC len 00017624 (93 KiB): IRAM[0m
[0;32mI (233) cpu_start: Pro cpu start user code[0m
[0;32mI (251) cpu_start: Starting scheduler on PRO CPU.[0m
[0;32mI (0) cpu_start: Starting scheduler on APP CPU.[0m
[0;32mI (0) gpio: GPIO[0]| InputEn: 1| OutputEn: 0| OpenDrain: 0| Pullup: 1| Pulldown: 0| Intr:3 [0m
短按触发回调 ... 
长按触发回调 ... 
短按触发回调 ... 
短按触发回调 ... 
短按触发回调 ... 
短按触发回调 ... 
```

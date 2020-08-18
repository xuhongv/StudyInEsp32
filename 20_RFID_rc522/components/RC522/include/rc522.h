#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "driver/spi_master.h"
#include "driver/spi_common_internal.h"

typedef void(*rc522_tag_callback_t)(uint8_t*);

typedef struct {
    int miso_io;
    int mosi_io;
    int sck_io;
    int sda_io;
    rc522_tag_callback_t callback;
} rc522_start_args_t;

esp_err_t rc522_spi_init(int miso_io, int mosi_io, int sck_io, int sda_io);
esp_err_t rc522_write_n(uint8_t addr, uint8_t n, uint8_t *data);
esp_err_t rc522_write(uint8_t addr , uint8_t val);
uint8_t* rc522_read_n(uint8_t addr, uint8_t n) ;
uint8_t rc522_read(uint8_t addr);
#define rc522_fw_version() rc522_read(0x37)
esp_err_t rc522_init();

esp_err_t rc522_set_bitmask(uint8_t addr, uint8_t mask);
esp_err_t rc522_clear_bitmask(uint8_t addr, uint8_t mask);
esp_err_t rc522_antenna_on();
uint8_t* rc522_calculate_crc(uint8_t *data, uint8_t n);
uint8_t* rc522_card_write(uint8_t cmd, uint8_t *data, uint8_t n, uint8_t* res_n);
uint8_t* rc522_request(uint8_t* res_n);
uint8_t* rc522_anticoll();
uint8_t* rc522_get_tag();
esp_err_t rc522_start(rc522_start_args_t start_args);
esp_err_t rc522_resume();
esp_err_t rc522_pause();

#ifdef __cplusplus
}
#endif
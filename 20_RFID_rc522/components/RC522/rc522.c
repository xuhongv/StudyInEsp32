#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/spi_master.h"
#include "soc/gpio_struct.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "rc522.h"

spi_device_handle_t rc522_spi;
static esp_timer_handle_t rc522_timer;
static bool rc522_timer_running = false;

esp_err_t rc522_spi_init(int miso_io, int mosi_io, int sck_io, int sda_io)
{
    spi_bus_config_t buscfg = {
        .miso_io_num = miso_io,
        .mosi_io_num = mosi_io,
        .sclk_io_num = sck_io,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1};

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 5000000,
        .mode = 0,
        .spics_io_num = sda_io,
        .queue_size = 7,
        .flags = SPI_DEVICE_HALFDUPLEX};

    esp_err_t ret;
#ifdef CONFIG_IDF_TARGET_ESP32
    ret = spi_bus_initialize(VSPI_HOST, &buscfg, 0);
#elif CONFIG_IDF_TARGET_ESP32S2
    ret = spi_bus_initialize(HSPI_HOST, &buscfg, 0);
#endif
    if (ret != ESP_OK)
    {
        return ret;
    }
#ifdef CONFIG_IDF_TARGET_ESP32
    ret = spi_bus_add_device(VSPI_HOST, &devcfg, &rc522_spi);
#elif CONFIG_IDF_TARGET_ESP32S2
    ret = spi_bus_add_device(HSPI_HOST, &devcfg, &rc522_spi);
#endif
    return ret;
}

esp_err_t rc522_write_n(uint8_t addr, uint8_t n, uint8_t *data)
{
    uint8_t *buffer = (uint8_t *)malloc(n + 1);
    buffer[0] = (addr << 1) & 0x7E;

    for (uint8_t i = 1; i <= n; i++)
    {
        buffer[i] = data[i - 1];
    }

    spi_transaction_t t;
    memset(&t, 0, sizeof(t));

    t.length = 8 * (n + 1);
    t.tx_buffer = buffer;

    esp_err_t ret = spi_device_transmit(rc522_spi, &t);

    free(buffer);

    return ret;
}

esp_err_t rc522_write(uint8_t addr, uint8_t val)
{
    return rc522_write_n(addr, 1, &val);
}

/* Returns pointer to dynamically allocated array of N places. */
uint8_t *rc522_read_n(uint8_t addr, uint8_t n)
{
    if (n <= 0)
    {
        return NULL;
    }

    spi_transaction_t t;
    memset(&t, 0, sizeof(t));

    uint8_t *buffer = (uint8_t *)malloc(n);

    t.flags = SPI_TRANS_USE_TXDATA;
    t.length = 8;
    t.tx_data[0] = ((addr << 1) & 0x7E) | 0x80;
    t.rxlength = 8 * n;
    t.rx_buffer = buffer;

    esp_err_t ret = spi_device_transmit(rc522_spi, &t);
    assert(ret == ESP_OK);

    return buffer;
}

uint8_t rc522_read(uint8_t addr)
{
    uint8_t *buffer = rc522_read_n(addr, 1);
    uint8_t res = buffer[0];
    free(buffer);

    return res;
}

esp_err_t rc522_init()
{
    // ---------- RW test ------------
    rc522_write(0x24, 0x25);
    assert(rc522_read(0x24) == 0x25);

    rc522_write(0x24, 0x26);
    assert(rc522_read(0x24) == 0x26);
    // ------- End of RW test --------

    rc522_write(0x01, 0x0F);
    rc522_write(0x2A, 0x8D);
    rc522_write(0x2B, 0x3E);
    rc522_write(0x2D, 0x1E);
    rc522_write(0x2C, 0x00);
    rc522_write(0x15, 0x40);
    rc522_write(0x11, 0x3D);

    rc522_antenna_on();

    printf("RC522 Firmware 0x%x\n", rc522_fw_version());

    return ESP_OK;
}

esp_err_t rc522_set_bitmask(uint8_t addr, uint8_t mask)
{
    return rc522_write(addr, rc522_read(addr) | mask);
}

esp_err_t rc522_clear_bitmask(uint8_t addr, uint8_t mask)
{
    return rc522_write(addr, rc522_read(addr) & ~mask);
}

esp_err_t rc522_antenna_on()
{
    esp_err_t ret;

    if (~(rc522_read(0x14) & 0x03))
    {
        ret = rc522_set_bitmask(0x14, 0x03);

        if (ret != ESP_OK)
        {
            return ret;
        }
    }

    return rc522_write(0x26, 0x60); // 43dB gain
}

/* Returns pointer to dynamically allocated array of two element */
uint8_t *rc522_calculate_crc(uint8_t *data, uint8_t n)
{
    rc522_clear_bitmask(0x05, 0x04);
    rc522_set_bitmask(0x0A, 0x80);

    rc522_write_n(0x09, n, data);

    rc522_write(0x01, 0x03);

    uint8_t i = 255;
    uint8_t nn = 0;

    for (;;)
    {
        nn = rc522_read(0x05);
        i--;

        if (!(i != 0 && !(nn & 0x04)))
        {
            break;
        }
    }

    uint8_t *res = (uint8_t *)malloc(2);

    res[0] = rc522_read(0x22);
    res[1] = rc522_read(0x21);

    return res;
}

uint8_t *rc522_card_write(uint8_t cmd, uint8_t *data, uint8_t n, uint8_t *res_n)
{
    uint8_t *result = NULL;
    uint8_t irq = 0x00;
    uint8_t irq_wait = 0x00;
    uint8_t last_bits = 0;
    uint8_t nn = 0;

    if (cmd == 0x0E)
    {
        irq = 0x12;
        irq_wait = 0x10;
    }
    else if (cmd == 0x0C)
    {
        irq = 0x77;
        irq_wait = 0x30;
    }

    rc522_write(0x02, irq | 0x80);
    rc522_clear_bitmask(0x04, 0x80);
    rc522_set_bitmask(0x0A, 0x80);
    rc522_write(0x01, 0x00);

    rc522_write_n(0x09, n, data);

    rc522_write(0x01, cmd);

    if (cmd == 0x0C)
    {
        rc522_set_bitmask(0x0D, 0x80);
    }

    uint16_t i = 1000;

    for (;;)
    {
        nn = rc522_read(0x04);
        i--;

        if (!(i != 0 && (((nn & 0x01) == 0) && ((nn & irq_wait) == 0))))
        {
            break;
        }
    }

    rc522_clear_bitmask(0x0D, 0x80);

    if (i != 0)
    {
        if ((rc522_read(0x06) & 0x1B) == 0x00)
        {
            if (cmd == 0x0C)
            {
                nn = rc522_read(0x0A);
                last_bits = rc522_read(0x0C) & 0x07;

                if (last_bits != 0)
                {
                    *res_n = (nn - 1) + last_bits;
                }
                else
                {
                    *res_n = nn;
                }

                result = (uint8_t *)malloc(*res_n);

                for (i = 0; i < *res_n; i++)
                {
                    result[i] = rc522_read(0x09);
                }
            }
        }
    }

    return result;
}

uint8_t *rc522_request(uint8_t *res_n)
{
    uint8_t *result = NULL;
    rc522_write(0x0D, 0x07);

    uint8_t req_mode = 0x26;
    result = rc522_card_write(0x0C, &req_mode, 1, res_n);

    if (*res_n * 8 != 0x10)
    {
        free(result);
        return NULL;
    }

    return result;
}

uint8_t *rc522_anticoll()
{
    uint8_t *result = NULL;
    uint8_t res_n;
    uint8_t serial_number[] = {0x93, 0x20};

    rc522_write(0x0D, 0x00);

    result = rc522_card_write(0x0C, serial_number, 2, &res_n);

    if (result != NULL && res_n != 5)
    {
        free(result);
        return NULL;
    }

    return result;
}

uint8_t *rc522_get_tag()
{
    uint8_t *result = NULL;
    uint8_t *res_data = NULL;
    uint8_t res_data_n;

    res_data = rc522_request(&res_data_n);

    if (res_data != NULL)
    {
        free(res_data);

        result = rc522_anticoll(&res_data_n);

        if (result != NULL)
        {
            uint8_t buf[] = {0x50, 0x00, 0x00, 0x00};
            uint8_t *crc = rc522_calculate_crc(buf, 2);

            buf[2] = crc[0];
            buf[3] = crc[1];

            free(crc);

            res_data = rc522_card_write(0x0C, buf, 4, &res_data_n);
            free(res_data);

            rc522_clear_bitmask(0x08, 0x08);

            return result;
        }
    }

    return NULL;
}

static void rc522_timer_callback(void *arg)
{
    uint8_t *serial_no = rc522_get_tag();
    
    if (serial_no != NULL)
    {
        rc522_tag_callback_t cb = (rc522_tag_callback_t)arg;
        cb(serial_no);
        free(serial_no);
    }
}

esp_err_t rc522_start(rc522_start_args_t start_args)
{
    assert(rc522_spi_init(
               start_args.miso_io,
               start_args.mosi_io,
               start_args.sck_io,
               start_args.sda_io) == ESP_OK);

    rc522_init();

    const esp_timer_create_args_t timer_args = {
        .callback = &rc522_timer_callback,
        .arg = (void *)start_args.callback};

    esp_err_t ret = esp_timer_create(&timer_args, &rc522_timer);

    if (ret != ESP_OK)
    {
        return ret;
    }

    return rc522_resume();
}

esp_err_t rc522_resume()
{
    return rc522_timer_running ? ESP_OK : esp_timer_start_periodic(rc522_timer, 125000);
}

esp_err_t rc522_pause()
{
    return !rc522_timer_running ? ESP_OK : esp_timer_stop(rc522_timer);
}
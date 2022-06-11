/*
 * fpga_util.c
 *
 * Collection of utilities to interact with bitstreams loaded
 * on the iCE40 on the MCH badge and using the "standard" stuff
 * defined for the badge.
 *
 * Copyright (C) 2022  Sylvain Munaut
 * Copyritht (C) 2022  Frans Faase
 */

#include <stdbool.h>
#include <stdint.h>

#include <esp_err.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

#include "ice40.h"
#include "rp2040.h"

#include "fpga_util.h"


/* ---------------------------------------------------------------------------
 * FPGA IRQ
 * ------------------------------------------------------------------------ */

static SemaphoreHandle_t g_irq_trig;


static void
fpga_irq_hamdler(void* arg)
{
    xSemaphoreGiveFromISR(g_irq_trig, NULL);
}

esp_err_t
fpga_irq_setup(ICE40 *ice40)
{
    esp_err_t res;

    // Setup semaphore
    g_irq_trig = xSemaphoreCreateBinary();

    // Install handler
    res = gpio_isr_handler_add(ice40->pin_int, fpga_irq_hamdler, NULL);
    if (res != ESP_OK)
        return res;

    // Configure GPIO
    gpio_config_t io_conf = {
        .intr_type    = GPIO_INTR_NEGEDGE,
        .mode         = GPIO_MODE_INPUT,
        .pin_bit_mask = 1ULL << ice40->pin_int,
        .pull_down_en = 0,
        .pull_up_en   = 1,
    };

    res = gpio_config(&io_conf);
    if (res != ESP_OK)
        return res;

    return ESP_OK;
}

void
fpga_irq_cleanup(ICE40 *ice40)
{
    // Reconfigure GPIO
    gpio_config_t io_conf = {
        .intr_type    = GPIO_INTR_DISABLE,
        .mode         = GPIO_MODE_INPUT,
        .pin_bit_mask = 1ULL << ice40->pin_int,
        .pull_down_en = 0,
        .pull_up_en   = 1,
    };
    gpio_config(&io_conf);

    // Remove handler
    gpio_isr_handler_remove(ice40->pin_int);

    // Release semaphore
    vSemaphoreDelete(g_irq_trig);
}

bool
fpga_irq_triggered(void)
{
    return xSemaphoreTake(g_irq_trig, 0) == pdTRUE;
}


/* ---------------------------------------------------------------------------
 * Wishbone bridge
 * ------------------------------------------------------------------------ */

struct fpga_wb_cmdbuf {
    bool       done;
    uint8_t   *buf;
    int        len;
    int        used;
    int        rd_cnt;
    uint32_t **rd_ptr;
};

struct fpga_wb_cmdbuf *
fpga_wb_alloc(int n)
{
    struct fpga_wb_cmdbuf *cb;

    if (n > 511)
        return NULL;

    cb = calloc(1, sizeof(struct fpga_wb_cmdbuf));

    cb->len  = 1 + (n * 8 * sizeof(uint32_t));

    cb->buf = malloc(cb->len);
    cb->rd_ptr = malloc(64 * sizeof(uint32_t*));

    cb->buf[0] = SPI_CMD_WISHBONE;
    cb->used = 1;

    return cb;
}

void
fpga_wb_free(struct fpga_wb_cmdbuf *cb)
{
    if (!cb)
        return;

    free(cb->buf);
    free(cb->rd_ptr);
    free(cb);
}

bool
fpga_wb_queue_write(struct fpga_wb_cmdbuf *cb,
                    int dev, uint32_t addr, uint32_t val)
{
    // Limits
    if (cb->done)
        return false;
    if ((cb->len - cb->used) < 8)
        return false;

    // Dev sel & Mode (Write, Re-Address)
    cb->buf[cb->used++] = 0xc0 | (dev & 0xf);

    // Address
    cb->buf[cb->used++] = (addr >> 18) & 0xff;
    cb->buf[cb->used++] = (addr >> 10) & 0xff;
    cb->buf[cb->used++] = (addr >>  2) & 0xff;

    // Data
    cb->buf[cb->used++] = (val >> 24) & 0xff;
    cb->buf[cb->used++] = (val >> 16) & 0xff;
    cb->buf[cb->used++] = (val >>  8) & 0xff;
    cb->buf[cb->used++] = (val      ) & 0xff;

    // Done
    return true;
}

bool
fpga_wb_queue_read(struct fpga_wb_cmdbuf *cb,
                   int dev, uint32_t addr, uint32_t *val)
{
    // Limits
    if (cb->done)
        return false;
    if ((cb->len - cb->used) < 8)
        return false;
    if (cb->rd_cnt >= 64)
        return false;

    // Dev sel & Mode (Write, Re-Address)
    cb->buf[cb->used++] = 0x40 | (dev & 0xf);

    // Address
    cb->buf[cb->used++] = (addr >> 18) & 0xff;
    cb->buf[cb->used++] = (addr >> 10) & 0xff;
    cb->buf[cb->used++] = (addr >>  2) & 0xff;

    // Data
    cb->buf[cb->used++] = 0x00;
    cb->buf[cb->used++] = 0x00;
    cb->buf[cb->used++] = 0x00;
    cb->buf[cb->used++] = 0x00;
    cb->rd_ptr[cb->rd_cnt++] = val;

    // Done
    return true;
}

bool
fpga_wb_queue_write_burst(struct fpga_wb_cmdbuf *cb,
                          int dev, uint32_t addr, const uint32_t *val, int n, bool inc)
{
    // Limits
    if (cb->done)
        return false;
    if ((cb->len - cb->used) < (4 * (n+1)))
        return false;

    // Dev sel & Mode (Write, Burst)
    cb->buf[cb->used++] = 0x80 | (inc ? 0x20 : 0x00) | (dev & 0xf);

    // Address
    cb->buf[cb->used++] = (addr >> 18) & 0xff;
    cb->buf[cb->used++] = (addr >> 10) & 0xff;
    cb->buf[cb->used++] = (addr >>  2) & 0xff;

    // Data
    while (n--) {
        cb->buf[cb->used++] = (*val >> 24) & 0xff;
        cb->buf[cb->used++] = (*val >> 16) & 0xff;
        cb->buf[cb->used++] = (*val >>  8) & 0xff;
        cb->buf[cb->used++] = (*val      ) & 0xff;
        val++;
    }

    // Done (for good !)
    cb->done = true;
    return true;
}

bool
fpga_wb_queue_read_burst(struct fpga_wb_cmdbuf *cb,
                         int dev, uint32_t addr, uint32_t *val, int n, bool inc)
{
    // Limits
    if (cb->done)
        return false;
    if ((cb->len - cb->used) < (4 * (n+1)))
        return false;
    if ((cb->rd_cnt + n) > 64)
        return false;

    // Dev sel & Mode (Write, Burst)
    cb->buf[cb->used++] = (inc ? 0x20 : 0x00) | (dev & 0xf);

    // Address
    cb->buf[cb->used++] = (addr >> 18) & 0xff;
    cb->buf[cb->used++] = (addr >> 10) & 0xff;
    cb->buf[cb->used++] = (addr >>  2) & 0xff;

    // Data
    while (n--) {
        cb->buf[cb->used++] = 0x00;
        cb->buf[cb->used++] = 0x00;
        cb->buf[cb->used++] = 0x00;
        cb->buf[cb->used++] = 0x00;
        cb->rd_ptr[cb->rd_cnt++] = val;
        val++;
    }

    // Done (for good !)
    cb->done = true;
    return true;
}

bool
fpga_wb_exec(struct fpga_wb_cmdbuf *cb, ICE40* ice40)
{
    esp_err_t res;
    int l;

    // Execute transmit transaction with the request
    res = ice40_send(ice40, cb->buf, cb->used);
    if (res != ESP_OK)
        return false;

    // If there was no read, nothing else to do
    if (!cb->rd_cnt)
        return true;

    // Execute a half duplex transaction to get the read data back
    l = 2 + (cb->rd_cnt * 4);
    cb->buf[0] = SPI_CMD_RESP_ACK;

    res = ice40_transaction(ice40, cb->buf, l, cb->buf, l);
    if (res != ESP_OK)
        return false;

    // Fill data to requester
    for (int i=0; i<cb->rd_cnt; i++) {
        *(cb->rd_ptr[i]) = (
            (cb->buf[4*i+2] << 24) |
            (cb->buf[4*i+3] << 16) |
            (cb->buf[4*i+4] <<  8) |
            (cb->buf[4*i+5])
        );
    }

    return true;
}


/* ---------------------------------------------------------------------------
 * Button reports
 * ------------------------------------------------------------------------ */

static uint16_t g_btn_state = 0;

void
fpga_btn_reset(void)
{
    g_btn_state = 0;
}

esp_err_t
fpga_btn_forward_events(ICE40 *ice40, xQueueHandle buttonQueue)
{
    rp2040_input_message_t buttonMessage;

    while (xQueueReceive(buttonQueue, &buttonMessage, 0) == pdTRUE)
    {
        uint8_t  pin   = buttonMessage.input;
        bool     value = buttonMessage.state;
        uint16_t btn_mask = 0;

        switch(pin) {
            case RP2040_INPUT_JOYSTICK_DOWN:
                btn_mask = 1 << 0;
                break;
            case RP2040_INPUT_JOYSTICK_UP:
                btn_mask = 1 << 1;
                break;
            case RP2040_INPUT_JOYSTICK_LEFT:
                btn_mask = 1 << 2;
                break;
            case RP2040_INPUT_JOYSTICK_RIGHT:
                btn_mask = 1 << 3;
                break;
            case RP2040_INPUT_JOYSTICK_PRESS:
                btn_mask = 1 << 4;
                break;
            case RP2040_INPUT_BUTTON_HOME:
                btn_mask = 1 << 5;
                break;
            case RP2040_INPUT_BUTTON_MENU:
                btn_mask = 1 << 6;
                break;
            case RP2040_INPUT_BUTTON_SELECT:
                btn_mask = 1 << 7;
                break;
            case RP2040_INPUT_BUTTON_START:
                btn_mask = 1 << 8;
                break;
            case RP2040_INPUT_BUTTON_ACCEPT:
                btn_mask = 1 << 9;
                break;
            case RP2040_INPUT_BUTTON_BACK:
                btn_mask = 1 << 10;
            default:
                break;
        }

        if (btn_mask != 0)
        {
            if (value)
                g_btn_state |=  btn_mask;
            else
                g_btn_state &= ~btn_mask;

            uint8_t spi_message[5] = {
                SPI_CMD_BUTTON_REPORT,
                g_btn_state >> 8,
                g_btn_state & 0xff,
                btn_mask >> 8,
                btn_mask & 0xff,
            };

            esp_err_t res = ice40_send(ice40, spi_message, 5);
            if (res != ESP_OK)
                return res;
        }
    }

    return ESP_OK;
}

/*
 * fpga_util.h
 *
 * Copyright (C) 2022  Sylvain Munaut
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#include <esp_err.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include "ice40.h"


/* SPI protocol  ---------------------------------------------------------- */

/* Commands */
#define SPI_CMD_NOP1                0x00
#define SPI_CMD_WISHBONE            0xf0
#define SPI_CMD_LOOPBACK            0xf1
#define SPI_CMD_LCD_PASSTHROUGH     0xf2
#define SPI_CMD_BUTTON_REPORT       0xf4
#define SPI_CMD_FREAD_GET           0xf8
#define SPI_CMD_FREAD_PUT           0xf9
#define SPI_CMD_IRQ_ACK             0xfd
#define SPI_CMD_RESP_ACK            0xfe
#define SPI_CMD_NOP2                0xff


/* FPGA IRQ --------------------------------------------------------------- */

esp_err_t fpga_irq_setup(ICE40 *ice40);
void      fpga_irq_cleanup(ICE40 *ice40);
bool      fpga_irq_triggered(void);


/* Wishbone bridge -------------------------------------------------------- */

struct fpga_wb_cmdbuf;

struct fpga_wb_cmdbuf *fpga_wb_alloc(int n);
void fpga_wb_free(struct fpga_wb_cmdbuf *cb);

bool fpga_wb_queue_write(struct fpga_wb_cmdbuf *cb, int dev, uint32_t addr, uint32_t val);
bool fpga_wb_queue_read(struct fpga_wb_cmdbuf *cb, int dev, uint32_t addr, uint32_t *val);
bool fpga_wb_queue_write_burst(struct fpga_wb_cmdbuf *cb, int dev, uint32_t addr, const uint32_t *val, int n, bool inc);
bool fpga_wb_queue_read_burst(struct fpga_wb_cmdbuf *cb, int dev, uint32_t addr, uint32_t *val, int n, bool inc);

bool fpga_wb_exec(struct fpga_wb_cmdbuf *cb, ICE40* ice40);


/* Button reports --------------------------------------------------------- */

void      fpga_btn_reset(void);
esp_err_t fpga_btn_forward_events(ICE40 *ice40, xQueueHandle buttonQueue);

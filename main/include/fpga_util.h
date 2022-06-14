/*
 * fpga_util.h
 *
 * Copyright (C) 2022  Sylvain Munaut
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "ice40.h"


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

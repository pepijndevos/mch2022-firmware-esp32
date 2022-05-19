#include "audio.h"

#include "freertos/FreeRTOS.h"
#include "esp_system.h"
#include "driver/i2s.h"
#include "driver/rtc_io.h"

#include "audio_sample.h"

#include <stdio.h>

void _audio_init(int i2s_num) {
    i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_TX,
        .sample_rate = 16000,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB,
        .dma_buf_count = 8,
        .dma_buf_len = 64,
        .intr_alloc_flags = 0,
        .use_apll = false,
        .bits_per_chan = I2S_BITS_PER_SAMPLE_16BIT
    };

    i2s_driver_install(i2s_num, &i2s_config, 0, NULL);

    i2s_pin_config_t pin_config = {
        .mck_io_num = 0,
        .bck_io_num = 4,
        .ws_io_num = 12,
        .data_out_num = 13,
        .data_in_num = I2S_PIN_NO_CHANGE
    };

    i2s_set_pin(i2s_num, &pin_config);
}

void _audio_init_l(int i2s_num) {
    i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_TX,
        .sample_rate = 16000,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ALL_LEFT,
        .communication_format = I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB,
        .dma_buf_count = 8,
        .dma_buf_len = 64,
        .intr_alloc_flags = 0,
        .use_apll = false,
        .bits_per_chan = I2S_BITS_PER_SAMPLE_16BIT
    };

    i2s_driver_install(i2s_num, &i2s_config, 0, NULL);

    i2s_pin_config_t pin_config = {
        .mck_io_num = 0,
        .bck_io_num = 4,
        .ws_io_num = 12,
        .data_out_num = 13,
        .data_in_num = I2S_PIN_NO_CHANGE
    };

    i2s_set_pin(i2s_num, &pin_config);
}

void _audio_init_r(int i2s_num) {
    i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_TX,
        .sample_rate = 16000,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ALL_RIGHT,
        .communication_format = I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB,
        .dma_buf_count = 8,
        .dma_buf_len = 64,
        .intr_alloc_flags = 0,
        .use_apll = false,
        .bits_per_chan = I2S_BITS_PER_SAMPLE_16BIT
    };

    i2s_driver_install(i2s_num, &i2s_config, 0, NULL);

    i2s_pin_config_t pin_config = {
        .mck_io_num = 0,
        .bck_io_num = 4,
        .ws_io_num = 12,
        .data_out_num = 13,
        .data_in_num = I2S_PIN_NO_CHANGE
    };

    i2s_set_pin(i2s_num, &pin_config);
}

void audio_init() {
    _audio_init(0);
}

void audio_test(xQueueHandle buttonQueue, pax_buf_t* pax_buffer, ILI9341* ili9341) {
    i2s_driver_uninstall(0);
    _audio_init(0);

    pax_noclip(pax_buffer);
    pax_background(pax_buffer, 0x325aa8);
    pax_draw_text(pax_buffer, 0xFFFFFFFF, NULL, 18, 0, 20*0, "Playing audio...");
    ili9341_write(ili9341, pax_buffer->buf);
    size_t count;
    size_t position = 0;
    while (position < sizeof(sample_raw)) {
        size_t length = sizeof(sample_raw) - position;
        if (length > 256) length = 256;
        i2s_write(0, &sample_raw[position], length, &count, portMAX_DELAY);
        if (count != length) {
            printf("i2s_write_bytes: count (%d) != length (%d)\n", count, length);
            abort();
        }
        position += length;
    }
    i2s_zero_dma_buffer(0);

    i2s_driver_uninstall(0);
    _audio_init_l(0);

    pax_noclip(pax_buffer);
    pax_background(pax_buffer, 0x325aa8);
    pax_draw_text(pax_buffer, 0xFFFFFFFF, NULL, 18, 0, 20*0, "Playing audio L...");
    ili9341_write(ili9341, pax_buffer->buf);
    position = 0;
    while (position < sizeof(sample_raw)) {
        size_t length = sizeof(sample_raw) - position;
        if (length > 256) length = 256;
        i2s_write(0, &sample_raw[position], length, &count, portMAX_DELAY);
        if (count != length) {
            printf("i2s_write_bytes: count (%d) != length (%d)\n", count, length);
            abort();
        }
        position += length;
    }
    i2s_zero_dma_buffer(0);

    i2s_driver_uninstall(0);
    _audio_init_r(0);

    pax_noclip(pax_buffer);
    pax_background(pax_buffer, 0x325aa8);
    pax_draw_text(pax_buffer, 0xFFFFFFFF, NULL, 18, 0, 20*0, "Playing audio R...");
    ili9341_write(ili9341, pax_buffer->buf);
    position = 0;
    while (position < sizeof(sample_raw)) {
        size_t length = sizeof(sample_raw) - position;
        if (length > 256) length = 256;
        i2s_write(0, &sample_raw[position], length, &count, portMAX_DELAY);
        if (count != length) {
            printf("i2s_write_bytes: count (%d) != length (%d)\n", count, length);
            abort();
        }
        position += length;
    }
    i2s_zero_dma_buffer(0);

}

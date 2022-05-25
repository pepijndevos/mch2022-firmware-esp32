#include "audio.h"

#include "freertos/FreeRTOS.h"
#include "esp_system.h"
#include "driver/i2s.h"
#include "driver/rtc_io.h"

#include <stdio.h>

void _audio_init(int i2s_num) {
    i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_TX,
        .sample_rate = 8000,
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

void audio_init() {
    _audio_init(0);
}

extern const uint8_t boot_snd_start[] asm("_binary_boot_snd_start");
extern const uint8_t boot_snd_end[] asm("_binary_boot_snd_end");

void play_bootsound() {
    size_t count;
    size_t position = 0;
    size_t sample_length = (boot_snd_end - boot_snd_start);
    while (position < sample_length) {
        size_t remaining = sample_length - position;
        if (remaining > 256) remaining = 256;
        i2s_write(0, &boot_snd_start[position], remaining, &count, portMAX_DELAY);
        if (count != remaining) {
            printf("i2s_write_bytes: count (%d) != length (%d)\n", count, remaining);
            abort();
        }
        position += remaining;
    }
    i2s_zero_dma_buffer(0);
}

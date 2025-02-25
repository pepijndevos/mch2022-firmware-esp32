#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <driver/gpio.h>
#include "hardware.h"
#include "ili9341.h"
#include "ice40.h"
#include "rp2040.h"
#include "pax_gfx.h"

typedef bool (*test_fn)(uint32_t *rc);

bool test_wait_for_response(uint32_t *rc) {
    RP2040* rp2040 = get_rp2040();
    rp2040_input_message_t button_message = {0};
    if (rc != NULL) *rc = 0;
    while (1) {
        if (xQueueReceive(rp2040->queue, &button_message, portMAX_DELAY) == pdTRUE) {
            if (button_message.state) {
                switch(button_message.input) {
                    case RP2040_INPUT_BUTTON_HOME:
                    case RP2040_INPUT_BUTTON_MENU:
                    case RP2040_INPUT_BUTTON_BACK:
                        return false;
                    case RP2040_INPUT_BUTTON_ACCEPT:
                        if (rc != NULL) *rc = 1;
                        return true;
                    default:
                        break;
                }
            }
        }
    }
}

bool run_test(pax_buf_t* pax_buffer, const pax_font_t *font, ILI9341* ili9341, int line, const char *test_name, test_fn fn) {
    bool test_result;
    uint32_t rc;

    /* Test name */
    pax_draw_text(pax_buffer, 0xffffffff, font, 18, 0, 20*line, test_name);
    if (ili9341) ili9341_write(ili9341, pax_buffer->buf);

    /* Run the test */
    test_result = fn(&rc);

    /* Display result */
    if (!test_result) {
        /* Error */
        char buf[10];
        snprintf(buf, sizeof(buf), "%08x", rc);
        pax_draw_text(pax_buffer, 0xffff0000, font, 18, 200, 20*line, buf);
    } else {
        /* OK ! */
        pax_draw_text(pax_buffer, 0xff00ff00, font, 18, 200, 20*line, "      OK");
    }

    if (ili9341) ili9341_write(ili9341, pax_buffer->buf);

    /* Pass through the 'OK' status */
    return test_result;
}

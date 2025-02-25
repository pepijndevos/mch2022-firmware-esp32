#include <stdio.h>
#include <string.h>
#include <sdkconfig.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <esp_system.h>
#include <esp_err.h>
#include <esp_log.h>
#include "appfs.h"
#include "ili9341.h"
#include "pax_gfx.h"
#include "pax_codecs.h"
#include "menu.h"
#include "rp2040.h"
#include "launcher.h"
#include "settings.h"
#include "dev.h"
#include "fpga_download.h"
#include "hardware.h"
#include "file_browser.h"
#include "fpga_test.h"
#include "animation.h"
#include "button_test.h"
#include "adc_test.h"
#include "sao.h"

extern const uint8_t dev_png_start[] asm("_binary_dev_png_start");
extern const uint8_t dev_png_end[] asm("_binary_dev_png_end");

typedef enum action {
    ACTION_NONE,
    ACTION_BACK,
    ACTION_FPGA_DL,
    ACTION_FPGA_TEST,
    ACTION_FILE_BROWSER,
    ACTION_FILE_BROWSER_INT,
    ACTION_ANIMATION,
    ACTION_BUTTON_TEST,
    ACTION_ADC_TEST,
    ACTION_SAO
} menu_dev_action_t;

void render_dev_help(pax_buf_t* pax_buffer) {
    const pax_font_t *font = pax_get_font("saira regular");
    pax_background(pax_buffer, 0xFFFFFF);
    pax_noclip(pax_buffer);
    pax_draw_text(pax_buffer, 0xFF000000, font, 18, 5, 240 - 18, "[A] accept  [B] back");
}

void menu_dev(xQueueHandle buttonQueue, pax_buf_t* pax_buffer, ILI9341* ili9341) {
    menu_t* menu = menu_alloc("Development tools", 34, 18);
    
    menu->fgColor           = 0xFF000000;
    menu->bgColor           = 0xFFFFFFFF;
    menu->bgTextColor       = 0xFF000000;
    menu->selectedItemColor = 0xFFfec859;
    menu->borderColor       = 0xFFfa448c;
    menu->titleColor        = 0xFFfec859;
    menu->titleBgColor      = 0xFFfa448c;
    menu->scrollbarBgColor  = 0xFFCCCCCC;
    menu->scrollbarFgColor  = 0xFF555555;
     
    pax_buf_t icon_dev;
    pax_decode_png_buf(&icon_dev, (void*) dev_png_start, dev_png_end - dev_png_start, PAX_BUF_32_8888ARGB, 0);
    
    menu_set_icon(menu, &icon_dev);
    
    menu_insert_item(menu, "FPGA download mode", NULL, (void*) ACTION_FPGA_DL, -1);
    menu_insert_item(menu, "FPGA selftest", NULL, (void*) ACTION_FPGA_TEST, -1);
    menu_insert_item(menu, "File browser (SD card)", NULL, (void*) ACTION_FILE_BROWSER, -1);
    menu_insert_item(menu, "File browser (internal)", NULL, (void*) ACTION_FILE_BROWSER_INT, -1);
    menu_insert_item(menu, "Animation", NULL, (void*) ACTION_ANIMATION, -1);
    menu_insert_item(menu, "Button test", NULL, (void*) ACTION_BUTTON_TEST, -1);
    menu_insert_item(menu, "Analog inputs", NULL, (void*) ACTION_ADC_TEST, -1);
    menu_insert_item(menu, "SAO", NULL, (void*) ACTION_SAO, -1);

    bool render = true;
    menu_dev_action_t action = ACTION_NONE;
    
    render_dev_help(pax_buffer);

    while (1) {
        rp2040_input_message_t buttonMessage = {0};
        if (xQueueReceive(buttonQueue, &buttonMessage, 16 / portTICK_PERIOD_MS) == pdTRUE) {
            uint8_t pin = buttonMessage.input;
            bool value = buttonMessage.state;
            switch(pin) {
                case RP2040_INPUT_JOYSTICK_DOWN:
                    if (value) {
                        menu_navigate_next(menu);
                        render = true;
                    }
                    break;
                case RP2040_INPUT_JOYSTICK_UP:
                    if (value) {
                        menu_navigate_previous(menu);
                        render = true;
                    }
                    break;
                case RP2040_INPUT_BUTTON_HOME:
                case RP2040_INPUT_BUTTON_BACK:
                    if (value) {
                        action = ACTION_BACK;
                    }
                    break;
                case RP2040_INPUT_BUTTON_ACCEPT:
                case RP2040_INPUT_JOYSTICK_PRESS:
                case RP2040_INPUT_BUTTON_SELECT:
                case RP2040_INPUT_BUTTON_START:
                    if (value) {
                        action = (menu_dev_action_t) menu_get_callback_args(menu, menu_get_position(menu));
                    }
                    break;
                default:
                    break;
            }
        }

        if (render) {
            menu_render(pax_buffer, menu, 0, 0, 320, 220, 0xFF491d88);
            ili9341_write(ili9341, pax_buffer->buf);
            render = false;
        }

        if (action != ACTION_NONE) {
            if (action == ACTION_FPGA_DL) {
                fpga_download(buttonQueue, get_ice40(), pax_buffer, ili9341);
            } else if (action == ACTION_FPGA_TEST) {
                fpga_test(buttonQueue, pax_buffer, ili9341);
            } else if (action == ACTION_FILE_BROWSER) {
                file_browser(buttonQueue, pax_buffer, ili9341, "/sd");
            } else if (action == ACTION_FILE_BROWSER_INT) {
                file_browser(buttonQueue, pax_buffer, ili9341, "/internal");
            } else if (action == ACTION_ANIMATION) {
                display_animation(pax_buffer, ili9341);
            } else if (action == ACTION_BUTTON_TEST) {
                test_buttons(buttonQueue, pax_buffer, ili9341);
            } else if (action == ACTION_ADC_TEST) {
                test_adc(buttonQueue, pax_buffer, ili9341);
            } else if (action == ACTION_SAO) {
                menu_sao(buttonQueue, pax_buffer, ili9341);
            } else if (action == ACTION_BACK) {
                break;
            }
            action = ACTION_NONE;
            render = true;
            render_dev_help(pax_buffer);
        }
    }

    menu_free(menu);
    
    pax_buf_destroy(&icon_dev);
}

/*
* SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
*
* SPDX-License-Identifier: Unlicense OR CC0-1.0
*/

#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_check.h"
#include "bsp_board.h"
#include "bsp_codec.h"
#include "bsp_lcd.h"
#include "lvgl/lvgl.h"
#include "bsp_btn.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "ui_main.h"
#include "ui_sr.h"

static const char *TAG = "ui_main";

LV_FONT_DECLARE(font_en_16)

static void ui_after_boot(void)
{
    bsp_lcd_set_backlight(false);
}

void ui_main(void)
{
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_make(237, 238, 239), LV_STATE_DEFAULT);

    /* For speech animation */
    ui_sr_anim_init();

    ui_after_boot();
}

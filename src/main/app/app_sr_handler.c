/*
* SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
*
* SPDX-License-Identifier: Unlicense OR CC0-1.0
*/


#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "app_audio.h"
#include "app_sr.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_check.h"
#include "ui_sr.h"
#include "app_sr_handler.h"
#include "esp_afe_sr_iface.h"
#include "driver/gpio.h"
#include "bsp_lcd.h"
#include <math.h> 

#define TIME_NORMAL 2000
#define TIME_SHORT 1000
#define TIME_LONG 4000
#define TIME_FULLY 15000
static const char *TAG = "sr_handler";

void head_up(int t )
{
    // Puts up the bed (blue)
    ESP_LOGI(TAG, "head_up, length: %d", t);
    gpio_set_level(GPIO_HEAD_UP, true);
    vTaskDelay(t / portTICK_PERIOD_MS);
    gpio_set_level(GPIO_HEAD_UP, false);
}
void head_down(int t )
{
    // Puts down the bed (cyan)
    ESP_LOGI(TAG, "head_down, length: %d", t);
    gpio_set_level(GPIO_HEAD_DOWN, true);
    vTaskDelay(t / portTICK_PERIOD_MS);
    gpio_set_level(GPIO_HEAD_DOWN, false);
}

void bed_up(int t )
{
    // Puts up the bed (red)
    ESP_LOGI(TAG, "bed_up, length: %d", t);
    gpio_set_level(GPIO_BED_UP, true);
    vTaskDelay(t / portTICK_PERIOD_MS);
    gpio_set_level(GPIO_BED_UP, false);
}
void bed_down(int t )
{
    // Puts down the bed (purple)
    ESP_LOGI(TAG, "bed_down, length: %d", t);
    gpio_set_level(GPIO_BED_DOWN, true);
    vTaskDelay(t / portTICK_PERIOD_MS);
    gpio_set_level(GPIO_BED_DOWN, false);
}

void leg_up(int t )
{
    // Puts up the bed (green)
    ESP_LOGI(TAG, "leg_up, length: %d", t);
    gpio_set_level(GPIO_LEG_UP, true);
    vTaskDelay(t / portTICK_PERIOD_MS);
    gpio_set_level(GPIO_LEG_UP, false);
}
void leg_down(int t )
{
    // Puts down the bed (brown?)
    ESP_LOGI(TAG, "leg_down, length: %d", t);
    gpio_set_level(GPIO_LEG_DOWN, true);
    vTaskDelay(t / portTICK_PERIOD_MS);
    gpio_set_level(GPIO_LEG_DOWN, false);
}

void sr_handler_task(void *pvParam)
{
    QueueHandle_t xQueue = (QueueHandle_t) pvParam;

    while (true) {
        sr_result_t result;
        xQueueReceive(xQueue, &result, portMAX_DELAY);

        ESP_LOGI(TAG, "cmd:%d, wakemode:%d,state:%d", result.command_id, result.wakenet_mode, result.state);

        if (ESP_MN_STATE_TIMEOUT == result.state) {
            ESP_LOGW(TAG, "Timeout");
            sr_anim_set_text("Timeout");
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            sr_anim_stop();
            bsp_lcd_set_backlight(false);
            continue;
        }

        if (WAKENET_DETECTED == result.wakenet_mode) {
            bsp_lcd_set_backlight(true);
            sr_anim_start();
            app_audio_beep_play_start();
            sr_anim_set_text("Say command");
            continue;
        }

        if (ESP_MN_STATE_DETECTED & result.state) {

            if (result.command_id < 1 || result.command_id > 24)
            {
                continue;
            }
      
            switch (result.command_id){
                case 1:
                    sr_anim_set_text("Headrest Up");
                    head_up(TIME_NORMAL);
                    break;
                case 2:
                    sr_anim_set_text("Headrest Down");
                    head_down(TIME_NORMAL);
                    break;
                case 3:
                    sr_anim_set_text("Legrest Up");
                    leg_up(TIME_NORMAL);
                    break;
                case 4:
                    sr_anim_set_text("Legrest Down");
                    leg_down(TIME_NORMAL);
                    break;
                case 5:
                    sr_anim_set_text("Bed Up");
                    bed_up(TIME_NORMAL);
                    break;
                case 6:
                    sr_anim_set_text("Bed Down");
                    bed_down(TIME_NORMAL);
                    break;
                case 7:
                    sr_anim_set_text("Headrest A Little Up");
                    head_up(TIME_SHORT);
                    break;
                case 8:
                    sr_anim_set_text("Headrest A Little Down");
                    head_down(TIME_SHORT);
                    break;
                case 9:
                    sr_anim_set_text("Leg A Little Up");
                    leg_up(TIME_SHORT);
                    break;
                case 10:
                    sr_anim_set_text("Leg A Little Down");
                    leg_down(TIME_SHORT);
                    break;
                case 11:
                    sr_anim_set_text("Bed A Little Up");
                    bed_up(TIME_SHORT);
                    break;
                case 12:
                    sr_anim_set_text("Bed A Little Down");
                    bed_down(TIME_SHORT);
                    break;
                case 13:
                    sr_anim_set_text("Headrest Way Up");
                    head_up(TIME_LONG);
                    break;
                case 14:
                    sr_anim_set_text("Headrest Way Down");
                    head_down(TIME_LONG);
                    break;
                case 15:
                    sr_anim_set_text("Legrest Way Up");
                    leg_up(TIME_LONG);
                    break;
                case 16:
                    sr_anim_set_text("Legrest Way Down");
                    leg_down(TIME_LONG);
                    break;
                case 17:
                    sr_anim_set_text("Bed Way Up");
                    bed_up(TIME_LONG);
                    break;
                case 18:
                    sr_anim_set_text("Bed Way Down");
                    bed_down(TIME_LONG);
                    break;
                case 19:
                    sr_anim_set_text("Headrest Fully Up");
                    head_up(TIME_FULLY);
                    break;
                case 20:
                    sr_anim_set_text("Headrest Fully Down");
                    head_down(TIME_FULLY);
                    break;
                case 21:
                    sr_anim_set_text("Legrest Fully Up");
                    leg_up(TIME_FULLY);
                    break;
                case 22:
                    sr_anim_set_text("Legrest Fully Down");
                    leg_down(TIME_FULLY);
                    break;
                case 23:
                    sr_anim_set_text("Bed Fully Up");
                    bed_up(TIME_FULLY);
                    break;
                case 24:
                    sr_anim_set_text("Bed Fully Down");
                    bed_down(TIME_FULLY);
                    break;
            }

            ESP_LOGI(TAG, "Done command");
            sr_anim_stop();
            bsp_lcd_set_backlight(false);
        }
    }

    vTaskDelete(NULL);
}

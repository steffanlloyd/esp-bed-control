/*
* SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
*
* SPDX-License-Identifier: Unlicense OR CC0-1.0
*/


#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "driver/i2s.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"
#include "app_audio.h"
#include "app_sr.h"
#include "bsp_codec.h"
#include "bsp_i2s.h"
#include "esp_afe_sr_models.h"
#include "esp_mn_models.h"
#include "app_sr_handler.h"

#include "model_path.h"
#include "esp_mn_speech_commands.h"
#include "esp_process_sdkconfig.h"

#define I2S_CHANNEL_NUM     (2)

static const char *TAG = "app_sr";

static model_iface_data_t *model_data = NULL;
static const esp_mn_iface_t *multinet = NULL;
static const esp_afe_sr_iface_t *afe_handle = NULL;
static QueueHandle_t g_result_que = NULL;

static srmodel_list_t *models = NULL;

static FILE *fp = NULL;
static bool b_record_en = false;

static void audio_feed_task(void *pvParam)
{
    size_t bytes_read = 0;
    esp_afe_sr_data_t *afe_data = (esp_afe_sr_data_t *) pvParam;
    int audio_chunksize = afe_handle->get_feed_chunksize(afe_data);
    ESP_LOGI(TAG, "audio_chunksize=%d, feed_channel=%d", audio_chunksize, 3);
    /* Allocate audio buffer and check for result */
    int16_t *audio_buffer = heap_caps_malloc(audio_chunksize * sizeof(int16_t) * 3, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (NULL == audio_buffer) {
        esp_system_abort("No mem for audio buffer");
    }

    while (true) {
        /* Read audio data from I2S bus */
        i2s_read(I2S_NUM_0, audio_buffer, audio_chunksize * I2S_CHANNEL_NUM * sizeof(int16_t), &bytes_read, portMAX_DELAY);

        /* Save audio data to file if record enabled */
        if (b_record_en && (NULL != fp)) {
            fwrite(audio_buffer, 1, audio_chunksize * I2S_CHANNEL_NUM * sizeof(int16_t), fp);
        }

        /* Channel Adjust */
        for (int  i = audio_chunksize - 1; i >= 0; i--) {
            audio_buffer[i * 3 + 2] = 0;
            audio_buffer[i * 3 + 1] = audio_buffer[i * 2 + 1];
            audio_buffer[i * 3 + 0] = audio_buffer[i * 2 + 0];
        }

        /* Feed samples of an audio stream to the AFE_SR */
        afe_handle->feed(afe_data, audio_buffer);
    }

    /* Clean up if audio feed ends */
    afe_handle->destroy(afe_data);

    /* Task never returns */
    vTaskDelete(NULL);
}

static void audio_detect_task(void *pvParam)
{
    bool detect_flag = false;
    esp_afe_sr_data_t *afe_data = (esp_afe_sr_data_t *) pvParam;

    /* Allocate buffer for detection */
    int afe_chunksize = afe_handle->get_fetch_chunksize(afe_data);

    int mu_chunksize = multinet->get_samp_chunksize(model_data);
    int chunk_num = multinet->get_samp_chunknum(model_data);
    assert(mu_chunksize == afe_chunksize);
    ESP_LOGI(TAG, "------------detect start------------\n");

    while (true) {
        afe_fetch_result_t* res = afe_handle->fetch(afe_data);
        if (!res || res->ret_value == ESP_FAIL) {
            ESP_LOGE(TAG, "fetch error!");
            //break;
        }

        if (res->wakeup_state == WAKENET_DETECTED) {
            ESP_LOGI(TAG, LOG_BOLD(LOG_COLOR_GREEN) "Wakeword detected");
            sr_result_t result = {
                .wakenet_mode = WAKENET_DETECTED,
                .state = ESP_MN_STATE_DETECTING,
                .command_id = 0,
            };
            xQueueSend(g_result_que, &result, 10);
        }
        else if (res->wakeup_state == WAKENET_CHANNEL_VERIFIED) {
            ESP_LOGI(TAG, LOG_BOLD(LOG_COLOR_GREEN) "Channel verified");
            detect_flag = true;
            afe_handle->disable_wakenet(afe_data);
        }

        if (true == detect_flag) {
            
            esp_mn_state_t mn_state = ESP_MN_STATE_DETECTING;
            if (false == app_audio_beep_is_playing()) {
                mn_state = multinet->detect(model_data, res->data);
            } else {
                continue;
            }

            if (ESP_MN_STATE_DETECTING == mn_state) {
                continue;
            }

            if (ESP_MN_STATE_TIMEOUT == mn_state) {
                ESP_LOGW(TAG, "Time out");
                sr_result_t result = {
                    .wakenet_mode = WAKENET_NO_DETECT,
                    .state = mn_state,
                    .command_id = 0,
                };
                xQueueSend(g_result_que, &result, 10);
                afe_handle->enable_wakenet(afe_data);
                detect_flag = false;
                continue;
            }

            if (ESP_MN_STATE_DETECTED == mn_state) {
                esp_mn_results_t *mn_result = multinet->get_results(model_data);
                for (int i = 0; i < mn_result->num; i++) {
                    ESP_LOGI(TAG, "TOP %d, command_id: %d, phrase_id: %d, prob: %f",
                             i + 1, mn_result->command_id[i], mn_result->phrase_id[i], mn_result->prob[i]);
                }

                int sr_command_id = mn_result->command_id[0];
                ESP_LOGI(TAG, "Deteted command : %d", sr_command_id);
                sr_result_t result = {
                    .wakenet_mode = WAKENET_NO_DETECT,
                    .state = mn_state,
                    .command_id = sr_command_id,
                };
                xQueueSend(g_result_que, &result, 10);
                afe_handle->enable_wakenet(afe_data);
                detect_flag = false;
                continue;
            }

            ESP_LOGE(TAG, "Exception unhandled");
        }
    }

    /* Clean up if audio feed ends */
    afe_handle->destroy(afe_data);

    /* Task never returns */
    vTaskDelete(NULL);
}

esp_err_t app_sr_start()
{
    g_result_que = xQueueCreate(1, sizeof(sr_result_t));
    ESP_RETURN_ON_FALSE(NULL != g_result_que, ESP_ERR_NO_MEM, TAG, "Failed create result queue");

    models = esp_srmodel_init("model");

    afe_handle = &ESP_AFE_SR_HANDLE;
    afe_config_t afe_config = AFE_CONFIG_DEFAULT();
    afe_config.wakenet_model_name = esp_srmodel_filter(models, ESP_WN_PREFIX, NULL);
    afe_config.aec_init = false;
    // afe_config.vad_init = false;
    ESP_LOGI(TAG, "load wakenet:%s", afe_config.wakenet_model_name);
    esp_afe_sr_data_t *afe_data = afe_handle->create_from_config(&afe_config);

    char *mn_name = esp_srmodel_filter(models, ESP_MN_PREFIX, NULL);
    multinet = esp_mn_handle_from_name(mn_name);
    ESP_LOGI(TAG, "load multinet:%s", mn_name);
    model_data = multinet->create(mn_name, 5760);

    esp_mn_commands_update_from_sdkconfig(multinet, model_data);

    BaseType_t ret_val = xTaskCreatePinnedToCore(audio_feed_task, "Feed Task", 4 * 1024, afe_data, 5, NULL, 1);
    ESP_RETURN_ON_FALSE(pdPASS == ret_val, ESP_FAIL, TAG,  "Failed create audio feed task");

    ret_val = xTaskCreatePinnedToCore(audio_detect_task, "Detect Task", 6 * 1024, afe_data, 5, NULL, 1);
    ESP_RETURN_ON_FALSE(pdPASS == ret_val, ESP_FAIL, TAG,  "Failed create audio detect task");

    ret_val = xTaskCreatePinnedToCore(sr_handler_task, "SR Handler Task", 4 * 1024, g_result_que, 1, NULL, 0);
    ESP_RETURN_ON_FALSE(pdPASS == ret_val, ESP_FAIL, TAG,  "Failed create audio handler task");

    return ESP_OK;
}

esp_err_t app_sr_reset_command_list(char *command_list)
{
    char *err_id = heap_caps_malloc(1024, MALLOC_CAP_SPIRAM);
    ESP_RETURN_ON_FALSE(NULL != err_id, ESP_ERR_NO_MEM, TAG,  "memory is not enough");
    //multinet->reset(model_data, command_list, err_id);
    free(err_id);
    return ESP_OK;
}

/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#pragma once

#include <stdbool.h>
#include "esp_err.h"
#include "esp_afe_sr_models.h"
#include "esp_mn_models.h"

#define SR_CONTINUE_DET 1

/**
 * @brief Speech command string
 *
 */
#define STR_DELIMITER       ";"

#define STR_TIMEOUT         "Timeout"
#define STR_WAKEWORD        "Hi ESP"
#define STR_LIGHT_RED       "Turn Red"
#define STR_LIGHT_GREEN     "Turn Green"
#define STR_LIGHT_BLUE      "Turn Blue"
#define STR_LIGHT_WHITE     "Turn White"
#define STR_LIGHT_ON        "Turn On The Light"
#define STR_LIGHT_OFF       "Turn Off The Light"
#define STR_LIGHT_COLOR     "Customize Color"

#define VOICE_LIGHT_RED     CONFIG_EN_SPEECH_COMMAND_ID0
#define VOICE_LIGHT_GREEN   CONFIG_EN_SPEECH_COMMAND_ID1
#define VOICE_LIGHT_BLUE    CONFIG_EN_SPEECH_COMMAND_ID2
#define VOICE_LIGHT_WHITE   CONFIG_EN_SPEECH_COMMAND_ID3
#define VOICE_LIGHT_ON      CONFIG_EN_SPEECH_COMMAND_ID4
#define VOICE_LIGHT_OFF     CONFIG_EN_SPEECH_COMMAND_ID5
#define VOICE_LIGHT_COLOR   CONFIG_EN_SPEECH_COMMAND_ID6

typedef struct {
    wakenet_state_t wakenet_mode;
    esp_mn_state_t state;
    int command_id;
} sr_result_t;

/**
 * @brief User defined command list
 *
 */
typedef enum {
    SR_CMD_SET_RED = 0,
    SR_CMD_SET_GREEN,
    SR_CMD_SET_BLUE,
    SR_CMD_SET_WHITE,
    SR_CMD_LIGHT_ON,
    SR_CMD_LIGHT_OFF,
    SR_CMD_CUSTOM_COLOR,
} sr_cmd_t;

/**
 * @brief Start speech recognition task
 *
 * @return
 *    - ESP_OK: Success
 *    - ESP_ERR_NO_MEM: No enough memory for speech recognition
 *    - Others: Fail
 */
esp_err_t app_sr_start();

/**
 * @brief Reset command list
 *
 * @param command_list New command string
 * @return
 *    - ESP_OK: Success
 *    - ESP_ERR_NO_MEM: No enough memory for err_id string
 *    - Others: Fail
 */
esp_err_t app_sr_reset_command_list(char *command_list);

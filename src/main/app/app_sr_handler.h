/*
 * SPDX-FileCopyrightText: 2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#pragma once

#include <stdbool.h>
#include "esp_err.h"

#define LED_RED 39
#define LED_GREEN 40
#define LED_BLUE 41
#define GPIO_HEAD_DOWN 10
#define GPIO_HEAD_UP 9
#define GPIO_LEG_DOWN 11
#define GPIO_LEG_UP 43
#define GPIO_BED_DOWN 13
#define GPIO_BED_UP 44

void sr_handler_task(void *pvParam);

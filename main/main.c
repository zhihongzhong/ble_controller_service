/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */
#include "bluetooth_controller.h"
#include "motor.h"
#include "task_temperature.h"
#include "task_hibernate.h"

void app_main(void)
{
    bluetooth_controller_init();
    temperature_task_init();    
}

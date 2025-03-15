/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "bluetooth_controller.h"
#include "motor.h"
void app_main(void)
{
    bluetooth_controller_handle_t bluetooth_controller_hdl;
    bluetooth_controller_init(&bluetooth_controller_hdl);
}

#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"

#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"
#include "esp_event.h"
#include "common.h"
#include"storage.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef enum {
    BLE_CTL_SERVICE, 
    BLE_CTL_CHAR_DECL_SPEED,    
    BLE_CTL_CHAR_VAL_SPEED,
    BLE_CTL_CHAR_DESC_SPEED,

    BLE_CTL_CHAR_DECL_MODE,
    BLE_CTL_CHAR_VAL_MODE,
    BLE_CTL_CHAR_DESC_MODE, 

    BLE_CTL_CHAR_DECL_TEMP,
    BLE_CTL_CHAR_VAL_TEMP,
    BLE_CTL_CHAR_DESC_TEMP,

    BLE_CTL_CHAR_DECL_COUNTDOWN, 
    BLE_CTL_CHAR_VAL_COUNTDOWN,
    BLE_CTL_CHAR_DESC_COUNTDOWN,
    
    BLE_CTL_CHAR_DECL_INST, 
    BLE_CTL_CHAR_VAL_INST,
    
    BLE_CTL_NUM_HANDLE,
} ble_ctl_handle_t;


esp_err_t bluetooth_controller_init();
esp_err_t bluetooth_controller_send_notification(ble_ctl_handle_t ble_ctl_handle, uint8_t* data, uint16_t len);
esp_err_t bluetooth_controller_get_attribute_value(ble_ctl_handle_t handle, uint16_t* length, const uint8_t** value);
esp_err_t bluetooth_controller_set_attribute_value(ble_ctl_handle_t handle, uint16_t length, uint8_t* value);

void uninitialize_connection();
bool is_bluetooth_controller_initialized();


#pragma once
#include "nvs_flash.h"


esp_err_t storage_init();
esp_err_t storage_set_data_u16(const char* key, uint16_t data);
esp_err_t storage_get_data_u16(const char* key, uint16_t* data);
esp_err_t storage_get_data_u8(const char* key, uint8_t* data);
esp_err_t storage_set_data_u8(const char* key, uint8_t data);
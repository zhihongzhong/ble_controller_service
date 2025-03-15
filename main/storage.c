#include "storage.h"


esp_err_t storage_init() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    return ret;
}

esp_err_t storage_set_data_u16(const char* key, uint16_t data) {
    nvs_handle_t my_handle;
    esp_err_t ret = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (ret != ESP_OK) return ret;
    ret = nvs_set_u16(my_handle, key, data);
    if (ret != ESP_OK) return ret;
    ret = nvs_commit(my_handle);
    if (ret != ESP_OK) return ret;
    nvs_close(my_handle);
    return ESP_OK;
}

esp_err_t storage_get_data_u16(const char* key, uint16_t* data) {
    nvs_handle_t my_handle;
    esp_err_t ret = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (ret != ESP_OK) return ret;
    ret = nvs_get_u16(my_handle, key, data);
    if (ret != ESP_OK) return ret;
    nvs_close(my_handle);
    return ESP_OK;
}

esp_err_t storage_get_data_u8(const char* key, uint8_t* data) {
    nvs_handle_t my_handle;
    esp_err_t ret = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (ret != ESP_OK) return ret;
    ret = nvs_get_u8(my_handle, key, data);
    if (ret != ESP_OK) return ret;
    nvs_close(my_handle);
    return ESP_OK;
}

void storage_getu8_or_default(const char* key, uint8_t* data, uint8_t default_value) {
    esp_err_t ret = storage_get_data_u8(key, data);
    if (ret != ESP_OK) {
        *data = default_value;
    }
}

esp_err_t storage_set_data_u8(const char* key, uint8_t data) {
    nvs_handle_t my_handle;
    esp_err_t ret = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (ret != ESP_OK) return ret;
    ret = nvs_set_u8(my_handle, key, data);
    if (ret != ESP_OK) return ret;
    ret = nvs_commit(my_handle);
    if (ret != ESP_OK) return ret;
    nvs_close(my_handle);
    return ESP_OK;
}
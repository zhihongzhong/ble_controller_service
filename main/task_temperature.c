#include "task_temperature.h"
#include "temperature.h"
#include "common.h"
#include "speed_calculator.h"

static TaskHandle_t temperature_task_hdl;

bool is_auto_mode_enabled()
{
    if( is_bluetooth_controller_initialized() == false ) return false;
    uint16_t length; 
    const uint8_t* value;
    bluetooth_controller_get_attribute_value(BLE_CTL_CHAR_VAL_MODE, &length, &value);
    return *value == 0x01;
}

bool is_notification_enabled()
{
    if( is_bluetooth_controller_initialized() == false ) return false;
    uint16_t length; 
    const uint8_t* value;
    bluetooth_controller_get_attribute_value(BLE_CTL_CHAR_DESC_TEMP, &length, &value);
    if( length == 2 ) 
    {
        uint16_t temp_client_conf = value[1] << 8 | value[0];
        if( temp_client_conf == 0x01 || temp_client_conf == 0x02)
        {
            return true;
        }
    }
    return false; 
}

void temperature_task(void* arg)
{

    while( !is_bluetooth_controller_initialized() ) {
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
    float humidity_val = 0.0, temperature_value = 0.0;
    for(;;)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        temperature_module_read(&temperature_value, &humidity_val);
        float motor_speedf = calculate_control_value(temperature_value, humidity_val);
        uint8_t motor_speed = (uint8_t)motor_speedf;
        uint8_t temp_val_8 = (uint8_t)temperature_value;
        ESP_LOGI(GATTS_TABLE_TAG, "Temperature: %.2f, Humidity: %.2f, Speed: %f ", temperature_value, humidity_val, motor_speedf);
        uint8_t temperature_and_humidity[2] = { (uint8_t)temp_val_8, (uint8_t)humidity_val};

        if( is_auto_mode_enabled() ) 
        {
            motor_set_speed(motor_speed);
        }
        if( is_bluetooth_controller_initialized() )
        {
            bluetooth_controller_set_attribute_value(BLE_CTL_CHAR_VAL_TEMP, sizeof(uint16_t), &temperature_and_humidity);
        }
        if( is_notification_enabled() )
        {
            esp_err_t ret = bluetooth_controller_send_notification(BLE_CTL_CHAR_VAL_TEMP, &temperature_and_humidity, sizeof(temperature_and_humidity));
            ESP_ERROR_CHECK(ret);
        }
    }
}


esp_err_t temperature_task_init()
{
    temperature_module_init();
    xTaskCreate(temperature_task, "temperature_task", 2048, NULL, 5, &temperature_task_hdl);
    return ESP_OK;
}

esp_err_t temperature_task_deinit()
{
    vTaskDelete(temperature_task_hdl);
    return ESP_OK;
}
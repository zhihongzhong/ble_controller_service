#include "task_temperature.h"
#include "driver/temperature_sensor.h"
#include "common.h"

static TaskHandle_t temperature_task_hdl;
static temperature_sensor_handle_t temp_sensor_hdl;

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
    float temp_val = 0;
    // install temperature sensor 
    temperature_sensor_config_t config = {
        .range_min = 20, 
        .range_max = 50,
        .clk_src = TEMPERATURE_SENSOR_CLK_SRC_DEFAULT,
    }; 
    ESP_ERROR_CHECK(temperature_sensor_install(&config, &temp_sensor_hdl));
    ESP_ERROR_CHECK(temperature_sensor_enable(temp_sensor_hdl));
    for(;;)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        temperature_sensor_get_celsius(temp_sensor_hdl, &temp_val); 
        uint8_t motor_speed = (temp_val - 20) / 30.0 * 100;
        uint8_t temp_val_8 = (uint8_t)temp_val;

        if( is_auto_mode_enabled() ) 
        {
            motor_set_speed(motor_speed);
        }
        if( is_bluetooth_controller_initialized() )
        {
            bluetooth_controller_set_attribute_value(BLE_CTL_CHAR_VAL_TEMP, sizeof(uint8_t), &temp_val_8);
        }
        if( is_notification_enabled() )
        {
            esp_err_t ret = bluetooth_controller_send_notification(BLE_CTL_CHAR_VAL_TEMP, &temp_val_8, sizeof(uint8_t));
            ESP_ERROR_CHECK(ret);
        }
    }
}

esp_err_t temperature_task_init()
{
    xTaskCreate(temperature_task, "temperature_task", 2048, NULL, 5, &temperature_task_hdl);
    return ESP_OK;
}

esp_err_t temperature_task_deinit()
{
    vTaskDelete(temperature_task_hdl);
    temperature_sensor_uninstall(temp_sensor_hdl);
    return ESP_OK;
}
#include "task_hibernate.h"
#include "bluetooth_controller.h"
#include "esp_timer.h"
#include "motor.h"
#include "esp_sleep.h"
#include "esp_check.h" 

static TaskHandle_t hibernate_task_hdl;

#define TIMER_WAKEUP_TIME_US (2 * 1000 * 1000) // 1 sec

void hibernate_system(); 

bool is_countdown_enabled() 
{
    if( is_bluetooth_controller_initialized() == false ) return false;
    return true; 
}
/**
 * @param arg - countdown time in seconds, to hibernate the system  
 * */ 
void hibernate_task(void *arg) 
{
    uint16_t countdown = (uint16_t)arg; 
    ESP_LOGI(GATTS_TABLE_TAG, "Hibernate system in %d seconds", countdown);
    uint16_t i = countdown; 
    for(; i > 0; i-- )
    {
        ESP_LOGI(GATTS_TABLE_TAG, "Hibernate in %d seconds", i);
        if( is_countdown_enabled() ) 
        {
            // send notification to bluetoothe client; 
            bluetooth_controller_send_notification(BLE_CTL_CHAR_VAL_COUNTDOWN, (uint8_t*)&i, sizeof(uint16_t));
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    bluetooth_controller_send_notification(BLE_CTL_CHAR_VAL_COUNTDOWN, (uint8_t*)i, sizeof(uint16_t));
    xTaskCreate(hibernate_system, "hibernate_system", 2048, NULL, 10, NULL);
    delete_hibernate_task_if_exists();
}

esp_err_t delete_hibernate_task_if_exists()
{
    if( hibernate_task_hdl != NULL ) 
    {
        vTaskDelete(hibernate_task_hdl);
        hibernate_task_hdl = NULL;
    }
    return ESP_OK;
};

esp_err_t create_hibernate_task(uint16_t countdown)
{
    // delete_hibernate_task_if_exists();
    return xTaskCreate(hibernate_task, "hibernate_task", 2048, (void*)countdown, 10, &hibernate_task_hdl);
}

esp_err_t register_timer_wakeup(void) 
{
    ESP_RETURN_ON_ERROR(esp_sleep_enable_timer_wakeup(TIMER_WAKEUP_TIME_US), GATTS_TABLE_TAG,"Configure timer as wakeup source failed");
    ESP_LOGI(GATTS_TABLE_TAG, "timer wakeup source is ready");
    return ESP_OK;
}

void hibernate_system() 
{
    // first of all, stop motor driver 
    motor_sleep(); 
    uninitialize_connection();
    for( ;; ) 
    {
        // go to sleep
        esp_light_sleep_start(); 
        ESP_LOGI(GATTS_TABLE_TAG, "Wake up from light sleep");
        // wait 2 second for devices to connect to. 
        // in the window period, the system is awake, 
        // if in the period no device connects, the system will fall asleep again. 
        vTaskDelay(1000 / portTICK_PERIOD_MS); 
        if( is_bluetooth_controller_initialized() )
        {
            break; 
        }
    }
    motor_wake(); 
    vTaskDelete(NULL);
}

esp_err_t hibernate_task_init() 
{
    return register_timer_wakeup();
}   
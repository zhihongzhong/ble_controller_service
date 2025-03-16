#include "common.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

// will be called in app_main. 
esp_err_t hibernate_task_init(); 
esp_err_t create_hibernate_task(uint16_t countdown); 

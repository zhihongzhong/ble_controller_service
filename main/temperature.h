#pragma once 
#include "esp_err.h"


esp_err_t temperature_module_init(); 
void temperature_module_read(float* temperature, float* humidity);
#pragma once
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "common.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_event.h"

#define MOTOR_SPEED_MAXIMUM 4095
#define MOTOR_SPEED_MIDDLE 2048
#define MOTOR_SPEED_MINIMUM 0

#define GPIO_OUTPUT_STBY GPIO_NUM_1
#define GPIO_OUTPUT_ENA GPIO_NUM_6
#define GPIO_OUTPUT_PIN_1 GPIO_NUM_7
#define GPIO_OUTPUT_PIN_2 GPIO_NUM_8
 

#define MOTOR_TIMER LEDC_TIMER_0
#define MOTOR_MODE LEDC_LOW_SPEED_MODE
#define MOTOR_DUTY_RES LEDC_TIMER_12_BIT // Set duty resolution to 12 bits
#define MOTOR_FREQUENCY (50)             // Frequency in Hertz. Set frequency at 50Hz
#define HIGH (1)
#define LOW (0)

typedef struct motor_config 
{
    uint32_t speed;
    uint32_t gpio_stby;
    uint32_t gpio_pin_1;
    uint32_t gpio_pin_2;
    uint32_t gpio_ena;
} motor_config_t;


typedef void* motor_handle_t;

#define MOTOR_CONFIG_DEFAULT()   \
{                                \
    .speed = MOTOR_SPEED_MIDDLE, \
    .gpio_stby = GPIO_OUTPUT_STBY, \
    .gpio_pin_1 = GPIO_OUTPUT_PIN_1, \
    .gpio_pin_2 = GPIO_OUTPUT_PIN_2, \
    .gpio_ena = GPIO_OUTPUT_ENA, \
}
esp_err_t motor_init(motor_config_t *config, motor_handle_t motor_hdl);
esp_err_t motor_set_speed(uint32_t speed);
esp_err_t motor_sleep();
esp_err_t motor_wake();

bool is_motor_initialized();
bool is_motor_sleeping();
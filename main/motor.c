#include "motor.h"

static esp_err_t init_gpio(uint8_t gpio)
{
    esp_err_t ret;
    gpio_config_t io_conf_1 = {
        .pin_bit_mask = (1ULL << gpio),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ret = gpio_config(&io_conf_1);
    return ret;
}

static esp_err_t init_pwm(ledc_channel_t channel, uint8_t gpio)
{
    esp_err_t ret;
    ledc_timer_config_t timer_conf = {
        .speed_mode = MOTOR_MODE,
        .duty_resolution = MOTOR_DUTY_RES,
        .timer_num = MOTOR_TIMER,
        .freq_hz = MOTOR_FREQUENCY, // Set output frequency at 50Hz
        .clk_cfg = LEDC_AUTO_CLK,
    };

    ret = ledc_timer_config(&timer_conf);
    if (ret)
    {
        return ret;
    }

    ledc_channel_config_t channel_conf = {
        .gpio_num = gpio,
        .speed_mode = MOTOR_MODE,
        .channel = channel,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = MOTOR_TIMER,
        .duty = 0, // Set duty to zero.
        .hpoint = 0,
    };

    ret = ledc_channel_config(&channel_conf);
    return ret;
}

esp_err_t motor_init(motor_config_t *config) 
{
    esp_err_t ret; 
    ret = init_gpio(GPIO_OUTPUT_STBY);
    if(ret != ESP_OK) {
        ESP_LOGE(GATTS_TABLE_TAG, "Error initializing GPIO %d", GPIO_OUTPUT_STBY);
        return ret;
    }
    ret = init_gpio(GPIO_OUTPUT_PIN_1);
    if (ret != ESP_OK) {
        ESP_LOGE(GATTS_TABLE_TAG, "Error initializing GPIO %d", GPIO_OUTPUT_PIN_1);
        return ret;
    }
    ret = init_gpio(GPIO_OUTPUT_PIN_2);
    if(ret != ESP_OK) {
        ESP_LOGE(GATTS_TABLE_TAG, "Error initializing GPIO %d", GPIO_OUTPUT_PIN_2);
        return ret;
    }
    ret = init_pwm(LEDC_CHANNEL_0, GPIO_OUTPUT_ENA);
    if (ret != ESP_OK) {
        ESP_LOGE(GATTS_TABLE_TAG, "Error initializing PWM %d", GPIO_OUTPUT_ENA);
        return ret;
    }
    gpio_set_level(GPIO_OUTPUT_PIN_1, LOW);
    gpio_set_level(GPIO_OUTPUT_PIN_2, LOW);
    gpio_set_level(GPIO_OUTPUT_STBY, HIGH);

    ret = storage_init();

    uint8_t speed; 
    storage_getu8_or_default(STORAGE_KEY_SPEED, &speed, 50);

    if( speed > 0 ) 
    {
        motor_set_speed(speed);
    }
    ESP_ERROR_CHECK(ret);

    return ESP_OK;
}

esp_err_t motor_set_speed(uint8_t speed)
{
    uint32_t amplfied_speed = speed * MOTOR_SPEED_MAXIMUM / 100;
    esp_err_t ret;
    ret = ledc_set_duty(MOTOR_MODE, LEDC_CHANNEL_0, amplfied_speed);
    if (ret != ESP_OK)
    {
        ESP_LOGE(GATTS_TABLE_TAG, "Error setting duty %d", (int)amplfied_speed);
        return ret;
    }
    ret = gpio_set_level(GPIO_OUTPUT_PIN_1, HIGH); 
    if( ret != ESP_OK ) 
    {
        ESP_LOGE(GATTS_TABLE_TAG, "Error setting GPIO %d", GPIO_OUTPUT_PIN_1);
        return ret;
    }
    ret = gpio_set_level(GPIO_OUTPUT_PIN_2, LOW);
    if( ret != ESP_OK ) 
    {
        ESP_LOGE(GATTS_TABLE_TAG, "Error setting GPIO %d", GPIO_OUTPUT_PIN_2);
        return ret;
    }
    ret = ledc_update_duty(MOTOR_MODE, LEDC_CHANNEL_0);
    if (ret != ESP_OK)
    {
        ESP_LOGE(GATTS_TABLE_TAG, "Error updating duty %d", (int)amplfied_speed);
        return ret;
    }
    return ESP_OK;
}


esp_err_t motor_sleep()
{
    esp_err_t ret;
    ret = gpio_set_level(GPIO_OUTPUT_STBY, LOW);
    if (ret != ESP_OK)
    {
        ESP_LOGE(GATTS_TABLE_TAG, "Error setting GPIO %d", GPIO_OUTPUT_STBY);
        return ret;
    }
    return ESP_OK;
}

esp_err_t motor_wake()
{
    esp_err_t ret;
    ret = gpio_set_level(GPIO_OUTPUT_STBY, HIGH);
    if (ret != ESP_OK)
    {
        ESP_LOGE(GATTS_TABLE_TAG, "Error setting GPIO %d", GPIO_OUTPUT_STBY);
        return ret;
    }
    return ESP_OK;
}

bool is_motor_initialized()
{
    return gpio_get_level(GPIO_OUTPUT_STBY) == HIGH;
}

bool is_motor_sleeping()
{
    return gpio_get_level(GPIO_OUTPUT_STBY) == LOW;
}
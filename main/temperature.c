#include <stdio.h>
#include "driver/i2c.h"
#include "esp_log.h"
#include "temperature.h"

#define I2C_MASTER_SCL_IO           GPIO_NUM_8     // I2C时钟引脚
#define I2C_MASTER_SDA_IO           GPIO_NUM_9     // I2C数据引脚
#define I2C_MASTER_FREQ_HZ          100000          // I2C主频
#define I2C_MASTER_NUM              I2C_NUM_0       // I2C端口号

#define AHT20_ADDR                  0x38            // AHT20 I2C地址
#define AHT20_INIT_CMD              0xBE            // 初始化命令
#define AHT20_MEASURE_CMD           0xAC            // 触发测量命令
#define AHT20_STATUS_BUSY           0x80            // 忙状态位

// I2C初始化
static void i2c_master_init() {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    i2c_param_config(I2C_MASTER_NUM, &conf);
    i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
}

// 读取AHT20状态
static uint8_t aht20_read_status() {
    uint8_t status;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (AHT20_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read_byte(cmd, &status, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return status;
}

// 初始化AHT20
static void aht20_init() {
    uint8_t cmd[3] = {AHT20_INIT_CMD, 0x08, 0x00};
    i2c_cmd_handle_t handle = i2c_cmd_link_create();
    i2c_master_start(handle);
    i2c_master_write_byte(handle, (AHT20_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write(handle, cmd, 3, true);
    i2c_master_stop(handle);
    i2c_master_cmd_begin(I2C_MASTER_NUM, handle, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(handle);
    vTaskDelay(10 / portTICK_PERIOD_MS);
}

// 读取温湿度
static void aht20_read(float *temperature, float *humidity) {
    uint8_t data[6] = {0};
    
    // 发送测量命令
    uint8_t trigger_cmd[3] = {AHT20_MEASURE_CMD, 0x33, 0x00};
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (AHT20_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write(cmd, trigger_cmd, 3, true);
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);

    // 等待测量完成
    while (aht20_read_status() & AHT20_STATUS_BUSY) {
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    // 读取数据
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (AHT20_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, data, 6, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);

    // 计算湿度
    uint32_t hum = ((data[1] << 16) | (data[2] << 8) | data[3]) >> 4;
    *humidity = (hum * 100.0) / (1 << 20);

    // 计算温度
    uint32_t temp = ((data[3] & 0x0F) << 16) | (data[4] << 8) | data[5];
    *temperature = (temp * 200.0) / (1 << 20) - 50;
}

void temperature_module_read(float* temperature, float* humidity)
{
    aht20_read(temperature, humidity);
}

esp_err_t temperature_module_init()
{
    i2c_master_init();
    aht20_init();
    return ESP_OK;
}
#include "bluetooth_controller.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include"storage.h"
#include "motor.h"
#include "driver/temperature_sensor.h"

static uint8_t adv_config_done = 0; 

static uint8_t raw_adv_data[] = {
    0x02, ESP_BLE_AD_TYPE_FLAG, 0x06,
    0x02, ESP_BLE_AD_TYPE_TX_PWR, 0xEB,
    0x03, ESP_BLE_AD_TYPE_16SRV_CMPL, 0xFF, 0x00,
    0x08, ESP_BLE_AD_TYPE_NAME_CMPL, 'B','L','E','_','F','A','N',
};

static uint8_t raw_scan_rsp_data[] = {
    /* Flags */
    0x02, ESP_BLE_AD_TYPE_FLAG, 0x06,
    /* TX Power Level */
    0x02, ESP_BLE_AD_TYPE_TX_PWR, 0xEB,
    /* Complete 16-bit Service UUIDs */
    0x03, ESP_BLE_AD_TYPE_16SRV_CMPL, 0xFF, 0x00,
    0x08, ESP_BLE_AD_TYPE_NAME_CMPL, 'B','L','E','_','F','A','N',
};

static esp_ble_adv_params_t adv_params = {
    .adv_int_min = 0x20,
    .adv_int_max = 0x40,
    .adv_type = ADV_TYPE_IND,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .channel_map = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY, 
};

#define ESP_APP_ID 0x56
#define NUM_HANDLE 8
/** define service */ 
const static uint16_t GATTS_SERVICE_UUID = 0x00FF;
/** define characteristic value */ 
const static uint16_t GATTS_CHAR_VAL_SPEED_UUID = 0xff01; 
const static uint16_t GATTS_CHAR_VAL_MODE_UUID = 0xff02;
const static uint16_t GATTS_CHAR_VAL_TEMP_UUID = 0xff03;

static const uint16_t primary_service_uuid = ESP_GATT_UUID_PRI_SERVICE;
static const uint16_t character_declaration_uuid = ESP_GATT_UUID_CHAR_DECLARE;
static const uint16_t character_client_config_uuid = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;

static const uint16_t char_prop_notify = ESP_GATT_CHAR_PROP_BIT_NOTIFY;
static const uint16_t char_prop_read_write = ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_READ;
// static const uint8_t char_prop_read_write_notify = ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY;

static TaskHandle_t temperature_task_hdl; 
static temperature_sensor_handle_t temp_sensor_hdl;
uint16_t gatts_handle_table[NUM_HANDLE];

uint8_t is_auto_mode_enabled = 0; 
bool is_bluetooth_initialized = false; 

typedef struct {
    uint8_t enabled; 
    uint16_t conn_id; 
    esp_gatt_if_t gatts_if;
    uint16_t char_handle;
} bluetooth_notification_conf_t; 

static bluetooth_notification_conf_t notification_conf = {
    .enabled = 0,
    .conn_id = 0,
    .gatts_if = 0,
    .char_handle = 0,
};

void init_notification_conf(uint8_t enabled, uint16_t conn_id, esp_gatt_if_t gatts_if, uint16_t char_handle)
{
    notification_conf.enabled = enabled;
    notification_conf.conn_id = conn_id;
    notification_conf.gatts_if = gatts_if;
    notification_conf.char_handle = char_handle;
}

void uninit_notification_conf()
{
    notification_conf.enabled = 0;
    notification_conf.conn_id = 0;
    notification_conf.gatts_if = 0;
    notification_conf.char_handle = 0;
}

bool is_notification_enabled()
{
    return notification_conf.enabled;
}

static void temperature_task(void* arg)
{

    while( !is_bluetooth_initialized ) {
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
        uint16_t temp_val_16 = (temp_val - 20) / 30.0 * 4095;
        uint8_t temp_val_8 = (uint8_t)temp_val;
        if( is_auto_mode_enabled ) 
        {
            motor_set_speed(temp_val_16);
        }
        esp_ble_gatts_set_attr_value(gatts_handle_table[6], sizeof(uint8_t), &temp_val_8);
        if( is_notification_enabled() )
        {
            esp_err_t ret = esp_ble_gatts_send_indicate(notification_conf.gatts_if, notification_conf.conn_id, notification_conf.char_handle, sizeof(uint8_t), &temp_val_8, false);
            ESP_ERROR_CHECK(ret);
        }
    }
}


#define CHAR_DECLARATION_SIZE (sizeof(uint8_t))
#define GATTS_DEMO_CHAR_VAL_LEN_MAX 500 

/**
 * characteristic table 
 * 0 - service declaration 
 * 1 - characteristic declaration, to control motor's speed, readonly
 * 2 - characteristic value, to control motor's speed, readwrite
 * 3 - characteristic declaration, to control motor's mode , readonly
 * 4 - characteristic value, to control motor's mode, readwrite, 0 - normal mode, 1 - auto mode
 * 5 - characteristic declaration, to show current temperature, readonly
 * 6 - characteristic value, to show current temperature, readonly(for client)
 * */ 
esp_err_t create_gatts_attr_db(esp_gatts_attr_db_t** gatt_db) 
{
    uint8_t speed_val;
    uint8_t mode_val;
    uint8_t temp_val = 0;
    uint8_t temp_client_conf = 0x000;

    esp_err_t ret = storage_get_data_u8("speed", &speed_val); 
    if( ret != ESP_OK )
    {
        // fallback speed value 
        speed_val = 128; 
    }
    ret = storage_get_data_u8("mode", &mode_val);
    if( ret != ESP_OK )
    {
        // fallback mode value 
        mode_val = 0; 
    }
    *gatt_db = calloc(NUM_HANDLE, sizeof(esp_gatts_attr_db_t));
    if( *gatt_db == NULL )
    {
        return ESP_ERR_NO_MEM;
    }
    // Service Declaration
    (*gatt_db)[0].attr_control.auto_rsp = ESP_GATT_AUTO_RSP;
    (*gatt_db)[0].att_desc = (esp_attr_desc_t) {
        .uuid_length = ESP_UUID_LEN_16,
        .uuid_p = (uint8_t*)&primary_service_uuid,
        .perm = ESP_GATT_PERM_READ,
        .max_length = sizeof(uint16_t),
        .length = sizeof(GATTS_SERVICE_UUID),
        .value = (uint8_t*)&GATTS_SERVICE_UUID
    };

    // Characteristic Declaration
    (*gatt_db)[1].attr_control.auto_rsp = ESP_GATT_AUTO_RSP;
    (*gatt_db)[1].att_desc = (esp_attr_desc_t) {
        .uuid_length = ESP_UUID_LEN_16,
        .uuid_p = (uint8_t*)&character_declaration_uuid,
        .perm = ESP_GATT_PERM_READ,
        .max_length = CHAR_DECLARATION_SIZE,
        .length = CHAR_DECLARATION_SIZE,
        .value = (uint8_t*)&char_prop_read_write,
    };
    // Characteristic Value - Speed
    (*gatt_db)[2].attr_control.auto_rsp = ESP_GATT_AUTO_RSP;
    (*gatt_db)[2].att_desc = (esp_attr_desc_t) {
        .uuid_length = ESP_UUID_LEN_16,
        .uuid_p = (uint8_t*)&GATTS_CHAR_VAL_SPEED_UUID,
        .perm = ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
        .max_length = GATTS_DEMO_CHAR_VAL_LEN_MAX,
        .length = sizeof(speed_val),
        .value = (uint8_t*)&speed_val
    };

    // Characteristic Declaration 
    (*gatt_db)[3].attr_control.auto_rsp = ESP_GATT_AUTO_RSP;
    (*gatt_db)[3].att_desc = (esp_attr_desc_t) {
        .uuid_length = ESP_UUID_LEN_16,
        .uuid_p = (uint8_t*)&character_declaration_uuid,
        .perm = ESP_GATT_PERM_READ,
        .max_length = CHAR_DECLARATION_SIZE,
        .length = CHAR_DECLARATION_SIZE,
        .value = (uint8_t*)&char_prop_read_write,
    };

    // Characteristic Value - Mode
    (*gatt_db)[4].attr_control.auto_rsp = ESP_GATT_AUTO_RSP;
    (*gatt_db)[4].att_desc = (esp_attr_desc_t) {
        .uuid_length = ESP_UUID_LEN_16,
        .uuid_p = (uint8_t*)&GATTS_CHAR_VAL_MODE_UUID,
        .perm = ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
        .max_length = GATTS_DEMO_CHAR_VAL_LEN_MAX,
        .length = sizeof(mode_val),
        .value = (uint8_t*)&mode_val
    };

    // Characteristic Declaration
    (*gatt_db)[5].attr_control.auto_rsp = ESP_GATT_AUTO_RSP;
    (*gatt_db)[5].att_desc = (esp_attr_desc_t) {
        .uuid_length = ESP_UUID_LEN_16,
        .uuid_p = (uint8_t*)&character_declaration_uuid,
        .perm = ESP_GATT_PERM_READ,
        .max_length = CHAR_DECLARATION_SIZE,
        .length = CHAR_DECLARATION_SIZE,
        .value = (uint8_t*)&char_prop_notify,
    };

    // Characteristic Value - Current Temperature 
    (*gatt_db)[6].attr_control.auto_rsp = ESP_GATT_AUTO_RSP;
    (*gatt_db)[6].att_desc = (esp_attr_desc_t) {
        .uuid_length = ESP_UUID_LEN_16,
        .uuid_p = (uint8_t*)&GATTS_CHAR_VAL_TEMP_UUID,
        .perm = ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
        .max_length = GATTS_DEMO_CHAR_VAL_LEN_MAX,
        .length = sizeof(temp_val),
        .value = (uint8_t*)&temp_val
    };
    // Characteristic Descriptor 
    (*gatt_db)[7].attr_control.auto_rsp = ESP_GATT_AUTO_RSP;
    (*gatt_db)[7].att_desc = (esp_attr_desc_t) {
        .uuid_length = ESP_UUID_LEN_16,
        .uuid_p = (uint8_t*)&character_client_config_uuid,
        .perm = ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
        .max_length = sizeof(uint8_t),
        .length = sizeof(temp_client_conf),
        .value = (uint8_t*)&temp_client_conf,
    };
    return ESP_OK;
}


#define ADV_CONFIG_FLAG (1 << 0)
#define SCAN_RSP_CONFIG_FLAG (1 << 1) 
static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event)
    {
        case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
            adv_config_done &= (~ADV_CONFIG_FLAG);
            if (adv_config_done == 0)
            {
                esp_ble_gap_start_advertising(&adv_params);
            }
            break;
        case ESP_GAP_BLE_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT:
            adv_config_done &= (~SCAN_RSP_CONFIG_FLAG);
            if (adv_config_done == 0)
            {
                esp_ble_gap_start_advertising(&adv_params);
            }
            break;
        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
            if( param->adv_data_cmpl.status != ESP_BT_STATUS_SUCCESS )
            {
                ESP_LOGE(GATTS_TABLE_TAG, "Advertising start failed\n");
            }
            else 
            {
                ESP_LOGI(GATTS_TABLE_TAG, "Advertising start successfully\n");
            }
            break;
        case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT: 
            if( param->adv_data_cmpl.status != ESP_BT_STATUS_SUCCESS )
            {
                ESP_LOGE(GATTS_TABLE_TAG, "Advertising stop failed\n");
            }
            else 
            {
                ESP_LOGI(GATTS_TABLE_TAG, "Stop adv successfully\n");
            }
            break; 
        case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
            ESP_LOGI(GATTS_TABLE_TAG, "update connetion params\n");
            break;
        default:
            break;
    }
}

static void gatts_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    switch( event ) 
    {
        case ESP_GATTS_REG_EVT:
            esp_err_t ret = esp_ble_gap_set_device_name("BLE_FAN");
            if ( ret )
            {
                ESP_LOGE(GATTS_TABLE_TAG, "set device name failed, error code = %x", ret);
            }
            esp_err_t raw_adv_ret = esp_ble_gap_config_adv_data_raw(raw_adv_data, sizeof(raw_adv_data));
            if ( raw_adv_ret )
            {
                ESP_LOGE(GATTS_TABLE_TAG, "config adv data failed, error code = %x", raw_adv_ret);
            }
            adv_config_done |= ADV_CONFIG_FLAG;
            esp_err_t raw_scan_ret = esp_ble_gap_config_scan_rsp_data_raw(raw_scan_rsp_data, sizeof(raw_scan_rsp_data));
            if ( raw_scan_ret )
            {
                ESP_LOGE(GATTS_TABLE_TAG, "config scan response data failed, error code = %x", raw_scan_ret);
            }
            adv_config_done |= SCAN_RSP_CONFIG_FLAG;
            esp_gatts_attr_db_t* gatt_db;
            esp_err_t create_db_ret = create_gatts_attr_db(&gatt_db);
            if ( create_db_ret )
            {
                ESP_LOGE(GATTS_TABLE_TAG, "create attribute table failed, error code = %x", create_db_ret);
            }
            esp_err_t create_attr_ret = esp_ble_gatts_create_attr_tab(gatt_db, gatts_if, NUM_HANDLE, 0);
            if ( create_attr_ret )
            {
                ESP_LOGE(GATTS_TABLE_TAG, "create attr table failed, error code = %x", create_attr_ret);
            }
            break;
        case ESP_GATTS_CREAT_ATTR_TAB_EVT:
            if( param->add_attr_tab.status != ESP_GATT_OK )
            {
                ESP_LOGE(GATTS_TABLE_TAG, "create attribute table failed, error code = %x", param->add_attr_tab.status);
            }
            else if( param->add_attr_tab.num_handle != NUM_HANDLE )
            {
                ESP_LOGE(GATTS_TABLE_TAG, "create attribute table abnormally, num_handle (%d)", param->add_attr_tab.num_handle);
            }
            else 
            {
                ESP_LOGI(GATTS_TABLE_TAG, "create attribute table successfully, the number handle = %d", param->add_attr_tab.num_handle);
                memcpy(gatts_handle_table, param->add_attr_tab.handles, sizeof(gatts_handle_table));
                esp_ble_gatts_start_service(gatts_handle_table[0]);
                is_bluetooth_initialized = true;
            }
            break;
        case ESP_GATTS_CONNECT_EVT: 
            ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_CONNECT_EVT, conn_id = %d", param->connect.conn_id);
            esp_log_buffer_hex(GATTS_TABLE_TAG, param->connect.remote_bda, 6);
            esp_ble_conn_update_params_t conn_params = {0};
            memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
            conn_params.latency = 0;
            conn_params.max_int = 0x20;
            conn_params.min_int = 0x10;
            conn_params.timeout = 400;
            esp_ble_gap_update_conn_params(&conn_params);
            break;
        case ESP_GATTS_DISCONNECT_EVT:
            ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_DISCONNECT_EVT, reason = %d", param->disconnect.reason);
            uninit_notification_conf();
            esp_ble_gap_start_advertising(&adv_params);
            break;
        case ESP_GATTS_WRITE_EVT:
            ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_WRITE_EVT, handle = %d", param->write.handle);
            ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_WRITE_EVT, is PREP = %d", param->write.is_prep);

            if( !param->write.is_prep )
            {
                ESP_LOGI(GATTS_TABLE_TAG, "GATT_WRITE_EVT, value len %d, value :", param->write.len);
                esp_log_buffer_hex(GATTS_TABLE_TAG, param->write.value, param->write.len);
                if( param->write.len == 1 && param->write.handle == gatts_handle_table[2] )
                {
                    const uint8_t* mode_val = NULL;
                    uint16_t length = 0;
                    esp_ble_gatts_get_attr_value(gatts_handle_table[4], &length, &mode_val);
                    if( *mode_val == 1) 
                    {
                        break;
                    }
                    uint8_t speed = param->write.value[0];
                    uint16_t motor_speed = (speed / 256.0) * 4095;
                    motor_set_speed(motor_speed); 
                    storage_set_data_u8("speed", speed);
                }
                else if( param->write.len == 1 && param->write.handle == gatts_handle_table[4] )
                {
                    uint8_t mode = param->write.value[0];
                    is_auto_mode_enabled = mode;
                    storage_set_data_u8("mode", mode);
                }
                else if( param->write.len == 2 && param->write.handle == gatts_handle_table[7])
                {
                    uint16_t temp_client_conf = param->write.value[1] << 8 | param->write.value[0];
                    ESP_LOGI(GATTS_TABLE_TAG, "temp_client_conf = %d", temp_client_conf);
                    if( temp_client_conf == 0x01 || temp_client_conf == 0x02)
                    {
                        init_notification_conf(1, param->write.conn_id, gatts_if, gatts_handle_table[6]);
                    }
                    else 
                    {
                        uninit_notification_conf();
                    }
                }
            }
            break;
        case ESP_GATTS_READ_EVT:
            ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_READ_EVT, handle = %d", param->read.handle);
            break;
        default:
            break;
    }
}

esp_err_t bluetooth_controller_init()
{
    esp_err_t ret; 

    ret = storage_init();
    ESP_ERROR_CHECK(ret);

    motor_config_t motor_config = MOTOR_CONFIG_DEFAULT();
    motor_handle_t motor_hdl;
    ret = motor_init(&motor_config, &motor_hdl);
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if( ret )
    {
        ESP_LOGE(GATTS_TABLE_TAG, "%s initialize controller failed\n", __func__);
        return ret;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if( ret )
    {
        ESP_LOGE(GATTS_TABLE_TAG, "%s enable controller failed\n", __func__);
        return ret;
    }

    esp_bluedroid_config_t bluedroid_cfg = BT_BLUEDROID_INIT_CONFIG_DEFAULT();
    ret = esp_bluedroid_init_with_cfg(&bluedroid_cfg);
    if( ret )
    {
        ESP_LOGE(GATTS_TABLE_TAG, "%s initialize bluedroid failed\n", __func__);
        return ret;
    }

    ret = esp_bluedroid_enable();
    if( ret )
    {
        ESP_LOGE(GATTS_TABLE_TAG, "%s enable bluedroid failed\n", __func__);
        return ret;
    }

    ret = esp_ble_gatts_register_callback(gatts_profile_event_handler);
    if( ret )
    {
        ESP_LOGE(GATTS_TABLE_TAG, "%s register gatts callback failed\n", __func__);
        return ret;
    }
    ret = esp_ble_gap_register_callback(gap_event_handler);
    if( ret )
    {
        ESP_LOGE(GATTS_TABLE_TAG, "%s register gap callback failed\n", __func__);
        return ret;
    }

    ret = esp_ble_gatts_app_register(ESP_APP_ID);
    if( ret )
    {
        ESP_LOGE(GATTS_TABLE_TAG, "%s register gatts app failed\n", __func__);
        return ret;
    }
    esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(500);
    if( local_mtu_ret )
    {
        ESP_LOGE(GATTS_TABLE_TAG, "%s set local MTU failed, error code = %x\n", __func__, local_mtu_ret);
    }

    xTaskCreate(temperature_task, "temperature_task", 2048, NULL, 5, &temperature_task_hdl);
    return ESP_OK;
}



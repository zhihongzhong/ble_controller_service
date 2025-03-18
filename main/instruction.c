#include "instruction.h"
#include "stdlib.h"
#include "bluetooth_controller.h"
#include "task_hibernate.h"
#include "motor.h"
#include "storage.h"

uint8_t* format_instruction(uint8_t* inst, uint16_t len) 
{
    return inst;
}

void free_instructions(uint8_t* inst) 
{
    free(inst); 
    inst = NULL;
}

esp_err_t parse_instruction(uint8_t *inst, uint16_t len)
{
    uint8_t *inst_big_endian = format_instruction(inst, len); 
    switch ( inst_big_endian[0] )
    {
        case INST_SPEED: 
        {
            uint8_t speed = inst_big_endian[1]; 
            const uint8_t* mode_val = NULL;
            uint16_t length = 0;
            bluetooth_controller_get_attribute_value(BLE_CTL_CHAR_VAL_MODE, &length, &mode_val);
            if( *mode_val == 0 ) 
            {
                motor_set_speed(speed); 
                storage_set_data_u8(STORAGE_KEY_SPEED, speed);
                bluetooth_controller_set_attribute_value(BLE_CTL_CHAR_VAL_SPEED, sizeof(uint8_t), &speed);
            }
            if( is_bluetooth_controller_initialized() && *mode_val == 0 ) 
            {
                bluetooth_controller_send_notification(BLE_CTL_CHAR_VAL_SPEED, &speed, sizeof( uint8_t ));
            }
            break; 
        }
        case INST_MODE:
        {
            uint8_t mode = inst_big_endian[1]; 
            storage_set_data_u8(STORAGE_KEY_MODE, mode);
            bluetooth_controller_set_attribute_value(BLE_CTL_CHAR_VAL_MODE, sizeof(uint8_t), &mode);
            if( mode == 0 ) 
            {
                const uint8_t* speed_val = NULL;
                uint16_t length = 0;
                bluetooth_controller_get_attribute_value(BLE_CTL_CHAR_VAL_MODE, &length, &speed_val);
                motor_set_speed(*speed_val); 
            }
            if( is_bluetooth_controller_initialized() )
            {
                bluetooth_controller_send_notification(BLE_CTL_CHAR_VAL_MODE, &mode, sizeof( uint8_t ));
            }
            break;  
        }
        case INST_SLEEP:
        {
            uint8_t is_cancel = inst_big_endian[1]; 
            uint16_t timeout = (uint16_t)inst_big_endian[2]; 
            if( is_cancel ) 
            {
                timeout = 0x00;
                delete_hibernate_task_if_exists(); 
            }
            else 
            {
                create_hibernate_task(timeout); 
            }
            bluetooth_controller_set_attribute_value(BLE_CTL_CHAR_VAL_COUNTDOWN, sizeof(uint16_t), &timeout);
            bluetooth_controller_send_notification(BLE_CTL_CHAR_VAL_COUNTDOWN, &timeout, sizeof( uint16_t ));
            break;
        }
        default: 
        // DO NOTHING
    }
    return ESP_OK; 
}
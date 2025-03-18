#pragma once 
#include "common.h"

esp_err_t parse_instruction(uint8_t *inst, uint16_t len); 


typedef enum {
    INST_SPEED, 
    INST_MODE,
    INST_SLEEP 
} instruction_t; 

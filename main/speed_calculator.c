#include <math.h>

#define ALPHA 0.8f      
#define BETA 0.2f       
#define POWER_K 1.5f   
#define MIN_INPUT 20.0f
#define MAX_INPUT 50.0f
#define MIN_OUTPUT 40.0f
#define MAX_OUTPUT 100.0f

float calculate_control_value(float temperature, float humidity) {
    float weighted = ALPHA * temperature + BETA * humidity;
    weighted = fmaxf(fminf(weighted, MAX_INPUT), MIN_INPUT);
    float normalized = (weighted - MIN_INPUT) / (MAX_INPUT - MIN_INPUT);
    float powered = powf(normalized, POWER_K);
    float output = MIN_OUTPUT + (MAX_OUTPUT - MIN_OUTPUT) * powered;
    return output;
}

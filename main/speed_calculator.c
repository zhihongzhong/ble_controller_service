#include <math.h>

#define ALPHA 0.8f      
#define BETA 0.2f       
#define POWER_K 1.5f   
#define MIN_INPUT 20.0f
#define MAX_INPUT 50.0f
#define MIN_OUTPUT 40.0f
#define MAX_OUTPUT 100.0f

float calculate_heat_index(float temerature_celsius, float humidity)
{
    float heat_index = 0.0f;
    float t = temerature_celsius;
    float rh = humidity;

    heat_index = -42.379 + 2.04901523 * t + 10.14333127 * rh - 0.22475541 * t * rh
                 - 6.83783e-03 * t * t - 5.481717e-02 * rh * rh
                 + 1.22874e-03 * t * t * rh + 8.5282e-04 * t * rh * rh
                 - 1.99e-06 * t * t * rh * rh;
    return heat_index;
}

float calculate_control_value(float temperature, float humidity) {
    float weighted = calculate_heat_index(temperature, humidity); 
    weighted = fmaxf(fminf(weighted, MAX_INPUT), MIN_INPUT);
    float normalized = (weighted - MIN_INPUT) / (MAX_INPUT - MIN_INPUT);
    float powered = powf(normalized, POWER_K);
    float output = MIN_OUTPUT + (MAX_OUTPUT - MIN_OUTPUT) * powered;
    return output;
}

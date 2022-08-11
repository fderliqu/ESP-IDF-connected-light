#include "mqtt_client.h"

extern uint16_t LEDstatus;
extern uint16_t COLORstatus;
extern char HEX_COLOR[7];
extern uint16_t BRIGHTNESS;

//Functions

esp_err_t mqtt_app_start();
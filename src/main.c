/*
    ESP-IDF-connected-light by Florian Derlique
    June 2022
    v0.0.3 : 
        --> ESP scan wifi AP and connect to ssid declared in DEFINES/GLOBAL
        --> [NEW] If ESP is disconnected, ESP try to reconnect, if reach max attempts, ESP restart scanning AP
*/

//----------------------------INCLUDES

#include <string.h>
#include <sys/unistd.h>

#include"esp_log.h"
#include"esp_event.h"
#include"nvs_flash.h"


//SPIFFS_FUNCTIONS
#include"Spiffs.h"
//WIFI_FUNCTIONS
#include"Wifi.h"
//SERVER_FUNCTION
#include"Server.h"
//MQTT_FUNCTION
#include"mqtt.h"
//LED_GPIO function
#include"LedGpio.h"

//----------------------------DEFINES/GLOBALS

bool server_is_started = false;

//----------------------------MAIN

void app_main() {
    //httpd_handle_t server = NULL;
    //Initialize NVS
    esp_err_t status = nvs_flash_init();
    if (status == ESP_ERR_NVS_NO_FREE_PAGES || status == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      status = nvs_flash_init();
    }
    ESP_ERROR_CHECK(status);
    //Init wifi with error check
    init_wifi();
    //Init spiffs
    /*
    ESP_ERROR_CHECK(init_spiffs());
    //Checking files
    ESP_ERROR_CHECK(print_spiffs_files());
    */
    //PWM_init
    //scan_i2c();
    init_pwm();
    //MQTT
    mqtt_app_start();
    //Loop
    while(1){
        /*
        if(status_connected && !server_is_started){
            
            server = start_webserver();
            if(server != NULL)server_is_started = true;
        }
        if(!status_connected && server_is_started){
            ESP_ERROR_CHECK(stop_webserver(server));
            server_is_started = false;
        }
        */
        //LED status control
        if(LEDstatus)ESP_ERROR_CHECK(OnLed());
        else ESP_ERROR_CHECK(OffLed());
        if(COLORstatus)SetColorValues(HEX_COLOR,BRIGHTNESS);
        else SetWhiteValues();
        sleep(1);
    }
}
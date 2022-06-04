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

//----------------------------DEFINES/GLOBALS

//----------------------------MAIN

void app_main() {
    //Initialize NVS
    esp_err_t status = nvs_flash_init();
    if (status == ESP_ERR_NVS_NO_FREE_PAGES || status == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      status = nvs_flash_init();
    }
    ESP_ERROR_CHECK(status);
    //Init wifi with error check
    ESP_ERROR_CHECK(init_wifi());
    //Loop
    while(1){
        if(!status_scan){
            while((status_scan = scan_wifi()) != true){
                ESP_LOGE(TAG_wifi_sta, "SSID : %s is not scanned",ESP_WIFI_SSID);
                sleep(1);
            }
        }
        //Connect when ESP find our ssid
        if(!status_connected && !ConnectionStart)connect_wifi();
        sleep(1);
    }
}
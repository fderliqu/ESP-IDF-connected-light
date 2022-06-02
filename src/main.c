//----------------------------INCLUDES

#include"esp_wifi.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"


//----------------------------DEFINES/GLOBALS
#define ESP_WIFI_SSID      "mywifissid"
#define ESP_WIFI_PASS      "mywifipassword"
#define ESP_WIFI_MAXIMUM_RETRY  10

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t wifi_sta_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

//ConnectionStart
static bool ConnectionStart = false;

//TAGS for ESP LOGGING
static const char *TAG_wifi_sta = "[ESP-IDF-connected_light]wifi_station";

//Wifi_station retry counter
static int wifi_sta_retry_num = 0;

//----------------------------EVENT_HANDLER

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data){
    //If ESP32 WiFi station start :
    if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START){
        //Do a connection if ConnectionStart is set
        if(ConnectionStart){
            ESP_LOGI(TAG_wifi_sta,"Connection attempt...");
            esp_wifi_connect();
        }
        else{
            ESP_LOGW(TAG_wifi_sta,"WiFi info is not set or not ready");
        }
    }
    //If ESP32 WiFi station disconnected to AP
    else if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START){
        //Try to reconnect if ConnectionStart is set
        if(ConnectionStart){
            //If connection attempt counter is under max connection attempt
            if(wifi_sta_retry_num < ESP_WIFI_MAXIMUM_RETRY){
                ESP_LOGI(TAG_wifi_sta,"Retry to connect to the AP");
                wifi_sta_retry_num++;
                esp_wifi_connect();
            }
            //Else set event group to WIFI_FAIL event
            else{
                xEventGroupSetBits(wifi_sta_event_group,WIFI_FAIL_BIT);
            }
            ESP_LOGI(TAG_wifi_sta,"connect to the AP failed");
        }
    }
    else if(event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP){
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG_wifi_sta, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        wifi_sta_retry_num = 0;
        xEventGroupSetBits(wifi_sta_event_group, WIFI_CONNECTED_BIT);
    }
}

//----------------------------WIFI_FUNCTION

//----------------------------SERVER_FUNCTION

//----------------------------SPIFFS_FUNCTION

//----------------------------MAIN

void app_main() {}
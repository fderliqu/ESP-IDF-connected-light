//----------------------------INCLUDES
#include <string.h>
#include <sys/unistd.h>

#include"esp_wifi.h"
#include"esp_log.h"
#include"esp_event.h"

#include"freertos/FreeRTOS.h"
#include"freertos/task.h"
#include"freertos/event_groups.h"
#include"nvs_flash.h"


#include"Wifi.h"

//----------------------------DEFINES/GLOBALS

//Instance event
esp_event_handler_instance_t instance_any_id;
esp_event_handler_instance_t instance_got_ip;

/* FreeRTOS event group to signal when we are connected*/
//static EventGroupHandle_t wifi_sta_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

//TAGS for ESP LOGGING
const char *TAG_wifi_sta = "[ESP-Wifi]";

//Wifi_station retry counter
static int wifi_sta_retry_num = 0;

static EventGroupHandle_t s_wifi_event_group;

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

//----------------------------EVENT_HANDLER

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data){
    //If ESP32 WiFi station start :
    if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START){
        ESP_LOGI(TAG_wifi_sta,"Connection attempt...");
        esp_wifi_connect();
    }
    //If ESP32 WiFi station disconnected to AP
    else if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED){
    //If connection attempt counter is under max connection attempt
        if(wifi_sta_retry_num < ESP_WIFI_MAXIMUM_RETRY){
            esp_wifi_connect();
            ESP_LOGI(TAG_wifi_sta,"Retry to connect to the AP");
            wifi_sta_retry_num++;
        }
            //Else set event group to WIFI_FAIL event
        else{
            ESP_LOGI(TAG_wifi_sta,"Can't connect to the AP %s",ESP_WIFI_SSID);
            xEventGroupSetBits(s_wifi_event_group,WIFI_FAIL_BIT);
        }    
    }
    else if(event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP){
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG_wifi_sta, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        wifi_sta_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

//----------------------------WIFI_FUNCTION



//INIT_WIFI

esp_err_t init_wifi(){

    s_wifi_event_group = xEventGroupCreate();

    //init TCP/IP pile
    ESP_ERROR_CHECK(esp_netif_init());

    //create loop event
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *netif_wifi_sta = esp_netif_create_default_wifi_sta();
    assert(netif_wifi_sta);

    //init default Wifi configuration
    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();

    //init Wifi with default config and start it
    ESP_ERROR_CHECK(esp_wifi_init(&config));

    //install event handlers
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,ESP_EVENT_ANY_ID,&event_handler,NULL,&instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,IP_EVENT_STA_GOT_IP,&event_handler,NULL,&instance_got_ip));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = ESP_WIFI_SSID,
            .password = ESP_WIFI_PASS,
            /* Setting a password implies station will connect to all security modes including WEP/WPA.
             * However these modes are deprecated and not advisable to be used. Incase your Access point
             * doesn't support WPA2, these mode can be enabled by commenting below line */
	     .threshold.authmode = WIFI_AUTH_WPA2_PSK,

            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );

    //start wifi with default config
    ESP_ERROR_CHECK(esp_wifi_start());
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    ESP_LOGI(TAG_wifi_sta,"fin attempt connect");
    // xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
    // happened.
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG_wifi_sta, "connected to ap SSID:%s password:%s",
                 ESP_WIFI_SSID, ESP_WIFI_PASS);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG_wifi_sta, "Failed to connect to SSID:%s, password:%s",
                 ESP_WIFI_SSID, ESP_WIFI_PASS);
    } else {
        ESP_LOGE(TAG_wifi_sta, "UNEXPECTED EVENT");
    }
    
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    
    vEventGroupDelete(s_wifi_event_group);
    
    return ESP_OK;
}
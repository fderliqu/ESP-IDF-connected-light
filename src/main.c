/*
    ESP-IDF-connected-light by Florian Derlique
    June 2022
    v0.0.3 : 
        --> ESP scan wifi AP and connect to ssid declared in DEFINES/GLOBAL
        --> [NEW] If ESP is disconnected, ESP try to reconnect, if reach max attempts, ESP restart scanning AP
*/

//----------------------------INCLUDES

#include <string.h>
#include <dirent.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include"esp_wifi.h"
#include"esp_log.h"
#include"esp_event.h"
#include "esp_spiffs.h"
#include"freertos/FreeRTOS.h"
#include"freertos/task.h"
#include"freertos/event_groups.h"
#include"nvs_flash.h"

//----------------------------DEFINES/GLOBALS
#define ESP_WIFI_SSID       "Redmi Note 10S"
#define ESP_WIFI_PASS       "fd3909op"
#define ESP_SPIFFS_PATH     "/spiffs"    
#define ESP_WIFI_MAXIMUM_RETRY  10
#define MAX_AP 16

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

//ConnectionStart
static bool ConnectionStart = false;
bool status_scan = false, status_connected = false;

//TAGS for ESP LOGGING
static const char *TAG_wifi_sta = "[ESP-Wifi]";
static const char *TAG_spiffs = "[ESP-SPIFFS]";
static const char *TAG_server = "[ESP-SERVER]";

//Wifi_station retry counter
static int wifi_sta_retry_num = 0;

//----------------------------DEBUG/INFOS FUNCTIONS

static void print_auth_mode(int authmode)
{
    switch (authmode) {
    case WIFI_AUTH_OPEN:
        ESP_LOGI(TAG_wifi_sta, "Authmode \tWIFI_AUTH_OPEN");
        break;
    case WIFI_AUTH_WEP:
        ESP_LOGI(TAG_wifi_sta, "Authmode \tWIFI_AUTH_WEP");
        break;
    case WIFI_AUTH_WPA_PSK:
        ESP_LOGI(TAG_wifi_sta, "Authmode \tWIFI_AUTH_WPA_PSK");
        break;
    case WIFI_AUTH_WPA2_PSK:
        ESP_LOGI(TAG_wifi_sta, "Authmode \tWIFI_AUTH_WPA2_PSK");
        break;
    case WIFI_AUTH_WPA_WPA2_PSK:
        ESP_LOGI(TAG_wifi_sta, "Authmode \tWIFI_AUTH_WPA_WPA2_PSK");
        break;
    case WIFI_AUTH_WPA2_ENTERPRISE:
        ESP_LOGI(TAG_wifi_sta, "Authmode \tWIFI_AUTH_WPA2_ENTERPRISE");
        break;
    case WIFI_AUTH_WPA3_PSK:
        ESP_LOGI(TAG_wifi_sta, "Authmode \tWIFI_AUTH_WPA3_PSK");
        break;
    case WIFI_AUTH_WPA2_WPA3_PSK:
        ESP_LOGI(TAG_wifi_sta, "Authmode \tWIFI_AUTH_WPA2_WPA3_PSK");
        break;
    default:
        ESP_LOGI(TAG_wifi_sta, "Authmode \tWIFI_AUTH_UNKNOWN");
        break;
    }
}

static void print_cipher_type(int pairwise_cipher, int group_cipher)
{
    switch (pairwise_cipher) {
    case WIFI_CIPHER_TYPE_NONE:
        ESP_LOGI(TAG_wifi_sta, "Pairwise Cipher \tWIFI_CIPHER_TYPE_NONE");
        break;
    case WIFI_CIPHER_TYPE_WEP40:
        ESP_LOGI(TAG_wifi_sta, "Pairwise Cipher \tWIFI_CIPHER_TYPE_WEP40");
        break;
    case WIFI_CIPHER_TYPE_WEP104:
        ESP_LOGI(TAG_wifi_sta, "Pairwise Cipher \tWIFI_CIPHER_TYPE_WEP104");
        break;
    case WIFI_CIPHER_TYPE_TKIP:
        ESP_LOGI(TAG_wifi_sta, "Pairwise Cipher \tWIFI_CIPHER_TYPE_TKIP");
        break;
    case WIFI_CIPHER_TYPE_CCMP:
        ESP_LOGI(TAG_wifi_sta, "Pairwise Cipher \tWIFI_CIPHER_TYPE_CCMP");
        break;
    case WIFI_CIPHER_TYPE_TKIP_CCMP:
        ESP_LOGI(TAG_wifi_sta, "Pairwise Cipher \tWIFI_CIPHER_TYPE_TKIP_CCMP");
        break;
    default:
        ESP_LOGI(TAG_wifi_sta, "Pairwise Cipher \tWIFI_CIPHER_TYPE_UNKNOWN");
        break;
    }

    switch (group_cipher) {
    case WIFI_CIPHER_TYPE_NONE:
        ESP_LOGI(TAG_wifi_sta, "Group Cipher \tWIFI_CIPHER_TYPE_NONE");
        break;
    case WIFI_CIPHER_TYPE_WEP40:
        ESP_LOGI(TAG_wifi_sta, "Group Cipher \tWIFI_CIPHER_TYPE_WEP40");
        break;
    case WIFI_CIPHER_TYPE_WEP104:
        ESP_LOGI(TAG_wifi_sta, "Group Cipher \tWIFI_CIPHER_TYPE_WEP104");
        break;
    case WIFI_CIPHER_TYPE_TKIP:
        ESP_LOGI(TAG_wifi_sta, "Group Cipher \tWIFI_CIPHER_TYPE_TKIP");
        break;
    case WIFI_CIPHER_TYPE_CCMP:
        ESP_LOGI(TAG_wifi_sta, "Group Cipher \tWIFI_CIPHER_TYPE_CCMP");
        break;
    case WIFI_CIPHER_TYPE_TKIP_CCMP:
        ESP_LOGI(TAG_wifi_sta, "Group Cipher \tWIFI_CIPHER_TYPE_TKIP_CCMP");
        break;
    default:
        ESP_LOGI(TAG_wifi_sta, "Group Cipher \tWIFI_CIPHER_TYPE_UNKNOWN");
        break;
    }
}

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
            ESP_LOGI(TAG_wifi_sta,"WiFi info is not set or not ready");
        }
    }
    //If ESP32 WiFi station disconnected to AP
    else if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED){
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
                status_scan = false;
                ConnectionStart = false;
                ESP_LOGI(TAG_wifi_sta,"Can't connect to the AP %s",ESP_WIFI_SSID);
                //xEventGroupSetBits(wifi_sta_event_group,WIFI_FAIL_BIT);
            }
            ESP_LOGI(TAG_wifi_sta,"connect to the AP failed");
            status_connected = false;
            
        }
    }
    else if(event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP){
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG_wifi_sta, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        wifi_sta_retry_num = 0;
        status_connected = true;
       // xEventGroupSetBits(wifi_sta_event_group, WIFI_CONNECTED_BIT);
    }
}

//----------------------------WIFI_FUNCTION

//INIT_WIFI

static esp_err_t init_wifi(){

    //wifi_sta_event_group = xEventGroupCreate();

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

    //Set STA mode
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    //start wifi with default config
    esp_err_t status = esp_wifi_start();
    ESP_ERROR_CHECK(status);

    return status;
}

static void restart_wifi(){
    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_ERROR_CHECK(esp_wifi_start());
}

static bool scan_wifi(){
    bool SSID_CHECK_SUCCES = false;
    uint16_t max_ap = MAX_AP, ap_count = 0;
    wifi_ap_record_t ap_info[MAX_AP]; 
    memset(ap_info,0,sizeof(ap_info));

    ESP_ERROR_CHECK(esp_wifi_scan_start(NULL,true));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&max_ap,ap_info));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));

    //List all the AP scanned
    ESP_LOGI(TAG_wifi_sta,"ESP scan %d AP :", ap_count);
    for(int i = 0; (i<MAX_AP)&&(i<ap_count); i++){
        ESP_LOGI(TAG_wifi_sta,"SSID\t%s",ap_info[i].ssid);
        ESP_LOGI(TAG_wifi_sta,"MAC_addr\t%02x:%02x:%02x:%02x:%02x:%02x",
            ap_info[i].bssid[0],
            ap_info[i].bssid[1],
            ap_info[i].bssid[2],
            ap_info[i].bssid[3],
            ap_info[i].bssid[4],
            ap_info[i].bssid[5]
        );
        ESP_LOGI(TAG_wifi_sta,"RSSI\t%d",ap_info[i].rssi);
        print_auth_mode(ap_info[i].authmode);
        if (ap_info[i].authmode != WIFI_AUTH_WEP) {
            print_cipher_type(ap_info[i].pairwise_cipher, ap_info[i].group_cipher);
        }
        ESP_LOGI(TAG_wifi_sta, "Channel \t\t%d\n", ap_info[i].primary);

        //If one of the ssid match with ESP_WIFI_SSID
        if(strcmp((char*)ap_info[i].ssid,ESP_WIFI_SSID) == 0){
            SSID_CHECK_SUCCES = true;
        }
    }
    return SSID_CHECK_SUCCES;
}

static void connect_wifi(){
    wifi_config_t config = {
        .sta = {
            .ssid = ESP_WIFI_SSID,
            .password = ESP_WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
            .capable = true,
            .required = false
            },
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &config));
    ConnectionStart = true;
    restart_wifi();

/*
    EventBits_t bits = xEventGroupWaitBits(wifi_sta_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

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
    
    vEventGroupDelete(wifi_sta_event_group);
    */
}
//----------------------------SERVER_FUNCTION

//----------------------------SPIFFS_FUNCTION

esp_err_t init_spiffs(){
    ESP_LOGI(TAG_spiffs,"Initialize SPIFFS");

    //Create config
    esp_vfs_spiffs_conf_t spiffs_config = {
        .base_path = ESP_SPIFFS_PATH,
        .partition_label = NULL,
        .max_files = 10,
        .format_if_mount_failed = true
    };
    esp_err_t status = esp_vfs_spiffs_register(&spiffs_config);
    if(status == ESP_FAIL)ESP_LOGE(TAG_spiffs,"Failed to mount or format filesystem");
    else if(status == ESP_ERR_NOT_FOUND)ESP_LOGE(TAG_spiffs,"Failed to find SPIFFS partition");
    else if(status == ESP_ERR_NO_MEM)ESP_LOGE(TAG_spiffs,"Failed to allocate objects");
    else if(status == ESP_ERR_INVALID_STATE)ESP_LOGE(TAG_spiffs,"Already mounted or partition is encrypted");
    if(!ESP_OK)return status;

    //Get infos
    size_t total = 0, used = 0;
    status = esp_spiffs_info(spiffs_config.partition_label,&total,&used);
    if(!ESP_OK){
        ESP_LOGE(TAG_spiffs,"Failed to get SPIFFS partition information (%s). Formatting...",esp_err_to_name(status));
        return status;
    }
    else{
        ESP_LOGI(TAG_spiffs, "Partition size: total: %d, used: %d", total, used);
    }

    //Check is used > total
    if(used>total){
        ESP_LOGE(TAG_spiffs,"Number of used bytes cannot be larger than total");
        return ESP_FAIL;
    }
    return status;
}

esp_err_t print_spiffs_files(){
    char * dirpath = ESP_SPIFFS_PATH;
    DIR *dir = opendir(dirpath);
    if(!dir){
        ESP_LOGE(TAG_spiffs,"Failed to open directory : %s",dirpath);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG_spiffs,"Checking spiffs files");
    struct dirent *entry;
    while((entry = readdir(dir)) != NULL){
        ESP_LOGI(TAG_spiffs,"Found : name : %s",entry->d_name);
    }
    return ESP_OK;
}

esp_err_t exist_file_spiffs(const char * file){
    char * dirpath = ESP_SPIFFS_PATH;
    bool check_status = false;
    DIR *dir = opendir(dirpath);
    esp_err_t status;
    if(!dir){
        ESP_LOGE(TAG_spiffs,"Failed to open directory : %s",dirpath);
        return ESP_FAIL;
    }
    struct dirent *entry;
    while(!check_status){
        if((entry = readdir(dir)) != NULL){
            if(strcmp(entry->d_name,file) == 0){
                closedir(dir);
                status = ESP_OK;
                check_status = true;
            }
        }
        else{
            closedir(dir);
            status =  ESP_FAIL;
            check_status = true;
        }
    }
    return status;
}


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
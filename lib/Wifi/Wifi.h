#include"esp_err.h"

#define ESP_WIFI_SSID       "Redmi Note 10S"
#define ESP_WIFI_PASS       "fd3909op"
  
#define ESP_WIFI_MAXIMUM_RETRY  10
#define MAX_AP 16

//TAGS for ESP LOGGING
extern const char *TAG_wifi_sta;

extern bool ConnectionStart,status_scan,status_connected;

esp_err_t init_wifi();
void restart_wifi();
bool scan_wifi();
void connect_wifi();
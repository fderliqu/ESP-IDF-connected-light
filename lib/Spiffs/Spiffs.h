//----------------------------DEFINES/GLOBALS

#include"esp_err.h"

#define ESP_SPIFFS_PATH     "/spiffs"  

//----------------------------PROTOTYPES

esp_err_t init_spiffs();
esp_err_t print_spiffs_files();
esp_err_t exist_file_spiffs(const char *);
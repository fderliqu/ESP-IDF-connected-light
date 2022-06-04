//----------------------------INCLUDES
#include<dirent.h>
#include <string.h>
#include <sys/unistd.h>

#include"esp_spiffs.h"
#include"esp_log.h"
#include"esp_event.h"

#include"Spiffs.h"

static const char *TAG_spiffs = "[ESP-SPIFFS]";

//----------------------------FUNCTIONS

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

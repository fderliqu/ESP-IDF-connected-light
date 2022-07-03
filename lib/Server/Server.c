//----------------------------INCLUDES
#include"esp_http_server.h"
#include"esp_wifi.h"
#include"esp_log.h"
#include"esp_event.h"
#include "driver/gpio.h"
#include"Server.h"

#include<stdio.h>
#include<string.h>

//----------------------------DEFINES

#define ESP_SERVER_PORT 80
#define BUFFER_SIZE 4096
#define ESP_SPIFFS_PATH     "/spiffs"  
#define MAX_SIZE_TYPE 10

#define GPIOLAMP GPIO_NUM_16

const char * TAG_server = "[ESP-Server]";

//----------------------------FUNCTION

esp_err_t handler_send_file(httpd_req_t *req){
    ESP_LOGI(TAG_server,"Enter to index handler, uri : %s",req->uri);
    char * buf = malloc(sizeof(char)*BUFFER_SIZE);
    size_t block_size;
    FILE* fd = NULL;
    if(strcmp(req->uri,"/") == 0){
        fd = fopen("/spiffs/index.html","r");
        httpd_resp_set_type(req,"text/html");
    }
    else if(strcmp(req->uri,"/w3.css") == 0){
        fd = fopen("/spiffs/w3.css","r");
        httpd_resp_set_type(req,"text/css");
    }
    else if(strcmp(req->uri,"/script.js") == 0){
        fd = fopen("/spiffs/script.js","r");
        httpd_resp_set_type(req,"text/javascript");
    }
    if(fd == NULL){
        ESP_LOGE(TAG_server,"Failed to open the file : %s",req->uri);
        return ESP_FAIL;
    }
    block_size = fread(buf,sizeof(char),BUFFER_SIZE,fd);
    while(block_size>0){
        if(httpd_resp_send_chunk(req,buf,block_size) != ESP_OK){
            ESP_LOGE(TAG_server,"Failed to send the chunk");
            fclose(fd);
            free(buf);
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
        block_size = fread(buf,sizeof(char),BUFFER_SIZE,fd);
    }
    fclose(fd);
    free(buf);
    ESP_LOGI(TAG_server,"Quit index handler, uri : %s     ",req->uri);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

esp_err_t handler_on(httpd_req_t *req){
    ESP_LOGI(TAG_server,"Enter to '/on' handler");
    const char resp[] = "ON";
    gpio_pad_select_gpio(GPIO_NUM_2);
    gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_2, 1);
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t handler_off(httpd_req_t *req){
    ESP_LOGI(TAG_server,"Enter to '/off' handler");
    const char resp[] = "OFF";
    gpio_pad_select_gpio(GPIO_NUM_2);
    gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_2, 0);
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t handler_get(httpd_req_t *req){
    esp_err_t status;
    if(strcmp(req->uri,"/on") == 0)status = handler_on(req);
    else if(strcmp(req->uri,"/off") == 0)status = handler_off(req);
    else status = handler_send_file(req);
    return status;
}

httpd_handle_t start_webserver(){
    httpd_handle_t server;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;
    config.server_port = ESP_SERVER_PORT;

    ESP_LOGI(TAG_server,"Starting Webserver on port %d",ESP_SERVER_PORT);
    if(httpd_start(&server,&config) != ESP_OK){
        ESP_LOGE(TAG_server,"Error Srating Webserver on port %d",ESP_SERVER_PORT);
        return NULL;
    }
    httpd_uri_t uri_get = {
        .uri = "/*",
        .method = HTTP_GET,
        .handler = handler_get,
        .user_ctx = NULL
    };
    ESP_LOGI(TAG_server,"Starting Webserver : Success... Registering URI handlers");
    httpd_register_uri_handler(server,&uri_get);
    return server;
}

esp_err_t stop_webserver(httpd_handle_t server){
    ESP_LOGI(TAG_server,"Stop Webserver");
    return httpd_stop(server);
}

esp_err_t index_redirection(httpd_req_t *req)
{
    httpd_resp_set_status(req, "307 Temporary Redirect");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}


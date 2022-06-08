//----------------------------INCLUDES
#include"esp_http_server.h"
#include"esp_wifi.h"
#include"esp_log.h"
#include"esp_event.h"

#include"Server.h"

//----------------------------DEFINES

#define ESP_SERVER_PORT 80

const char * TAG_server = "[ESP-Server]";

//----------------------------FUNCION

esp_err_t handler_index(httpd_req_t *req){
    const char resp[] = "INDEX";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t handler_on(httpd_req_t *req){
    const char resp[] = "ON";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t handler_off(httpd_req_t *req){
    return ESP_OK;
}

httpd_handle_t start_webserver(){
    httpd_handle_t server;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    ESP_LOGI(TAG_server,"Starting Webserver on port %d",ESP_SERVER_PORT);
    if(httpd_start(&server,&config) != ESP_OK){
        ESP_LOGE(TAG_server,"Error Srating Webserver on port %d",ESP_SERVER_PORT);
        return NULL;
    }
    httpd_uri_t uri_index = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = handler_index,
        .user_ctx = NULL
    };
    httpd_uri_t uri_on = {
        .uri = "/on",
        .method = HTTP_GET,
        .handler = handler_on,
        .user_ctx = NULL
    };
    httpd_uri_t uri_off = {
        .uri = "/off",
        .method = HTTP_GET,
        .handler = handler_off,
        .user_ctx = NULL
    };
    ESP_LOGI(TAG_server,"Starting Webserver : Success... Registering URI handlers");
    httpd_register_uri_handler(server,&uri_on);
    httpd_register_uri_handler(server,&uri_off);
    httpd_register_uri_handler(server,&uri_index);
    return server;
}

esp_err_t stop_webserver(httpd_handle_t server){
    ESP_LOGI(TAG_server,"Stop Webserver");
    return httpd_stop(server);
}


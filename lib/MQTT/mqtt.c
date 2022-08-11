#include "esp_log.h"
#include "esp_event.h"
#include "mqtt_client.h"
#include"string.h"
#include"stdlib.h"

#include "mqtt.h"

#define MQTT_URI "mqtt://mqtt.eclipseprojects.io"
#define SIZE_BUF 20
const char * TAG_mqtt = "[ESP-MQTT]";

uint16_t LEDstatus;
uint16_t COLORstatus;
char HEX_COLOR[7];
uint16_t RED;
uint16_t GREEN;
uint16_t BLUE;
uint16_t BRIGHTNESS;

static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    char buf[SIZE_BUF];
    char buf_brightness[SIZE_BUF];
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    // your_context_t *context = event->context;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG_mqtt, "MQTT_EVENT_CONNECTED");
            /* 
            msg_id = esp_mqtt_client_publish(client, "/topic/qos1", "data_3", 0, 1, 0);
            ESP_LOGI(TAG_mqtt, "sent publish successful, msg_id=%d", msg_id);

            msg_id = esp_mqtt_client_subscribe(client, "/topic/qos0", 0);
            ESP_LOGI(TAG_mqtt, "sent subscribe successful, msg_id=%d", msg_id);

            msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
            ESP_LOGI(TAG_mqtt, "sent subscribe successful, msg_id=%d", msg_id);

            msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
            ESP_LOGI(TAG_mqtt, "sent unsubscribe successful, msg_id=%d", msg_id);
            */
            /*
            msg_id = esp_mqtt_client_subscribe(client, "/topic/lamp/color/status", 0);
            ESP_LOGI(TAG_mqtt, "sent subscribe successful, msg_id=%d", msg_id);
            msg_id = esp_mqtt_client_subscribe(client, "/topic/lamp/color/value", 0);
            ESP_LOGI(TAG_mqtt, "sent subscribe successful, msg_id=%d", msg_id);
            */

            //LED STATUS
            memset(buf,0,SIZE_BUF);
            if(LEDstatus)memcpy(buf,"true\0",sizeof("true\0"));
            else memcpy(buf,"false\0",sizeof("false\0"));
            msg_id = esp_mqtt_client_publish(client,"/topic/lamp/onoff/status",buf, 0, 1, 0);
            ESP_LOGI(TAG_mqtt, "sent publish successful, msg_id=%d", msg_id);
            msg_id = esp_mqtt_client_subscribe(client, "/topic/lamp/onoff/status", 0);
            ESP_LOGI(TAG_mqtt, "sent subscribe successful, msg_id=%d", msg_id);

            //COLOR STATUS
            /*
            memset(buf,0,SIZE_BUF);
            if(COLORstatus)memcpy(buf,"true\0",sizeof("true\0"));
            else memcpy(buf,"false\0",sizeof("false\0"));
            msg_id = esp_mqtt_client_publish(client,"/topic/lamp/color/status",buf, 0, 1, 0);
            ESP_LOGI(TAG_mqtt, "sent publish successful, msg_id=%d", msg_id);
            */
            msg_id = esp_mqtt_client_subscribe(client, "/topic/lamp/color/status", 0);
            ESP_LOGI(TAG_mqtt, "sent subscribe successful, msg_id=%d", msg_id);

            //COLOR VALUE
            if(COLORstatus && LEDstatus){
                /*
                msg_id = esp_mqtt_client_publish(client,"/topic/lamp/color/value",HEX_COLOR, 0, 1, 0);
                ESP_LOGI(TAG_mqtt, "sent publish successful, msg_id=%d", msg_id);
                
                msg_id = esp_mqtt_client_subscribe(client, "/topic/lamp/color/value", 0);
                ESP_LOGI(TAG_mqtt, "sent subscribe successful, msg_id=%d", msg_id);*/
            }
/*
            //BRIGHTNESS VALUE
            if(LEDstatus){
                msg_id = esp_mqtt_client_publish(client,"/topic/lamp/brightness/value","tmp", 0, 1, 0);
                ESP_LOGI(TAG_mqtt, "sent publish successful, msg_id=%d", msg_id);
                msg_id = esp_mqtt_client_subscribe(client, "/topic/lamp/brightness/value", 0);
                ESP_LOGI(TAG_mqtt, "sent subscribe successful, msg_id=%d", msg_id);
            }
*/
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG_mqtt, "MQTT_EVENT_DISCONNECTED");
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG_mqtt, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            /*
            msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
            ESP_LOGI(TAG_mqtt, "sent publish successful, msg_id=%d", msg_id);
            */
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG_mqtt, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG_mqtt, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG_mqtt, "MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            if(!memcmp(event->topic,"/topic/lamp/onoff/status",event->topic_len)){
                if(!memcmp(event->data,"true",event->data_len))LEDstatus = 1;
                else LEDstatus = 0;
                ESP_LOGI(TAG_mqtt, "LED_status = %d",LEDstatus);
            }
            if(!memcmp(event->topic,"/topic/lamp/color/status",event->topic_len)){
                if(!memcmp(event->data,"true",event->data_len))COLORstatus = 1;
                else COLORstatus = 0;
                if (COLORstatus)esp_mqtt_client_subscribe(client, "/topic/lamp/color/value", 0);
                else msg_id = esp_mqtt_client_unsubscribe(client, "/topic/lamp/color/value");
                ESP_LOGI(TAG_mqtt, "COLOR_status = %d",COLORstatus);
            }
            if(!memcmp(event->topic,"/topic/lamp/color/value",event->topic_len)){
                memcpy(HEX_COLOR,event->data,event->data_len);
                ESP_LOGI(TAG_mqtt, "COLOR_value = %s",HEX_COLOR);
                uint16_t red,blue,green;
                int status = sscanf(HEX_COLOR, "#%02hx%02hx%02hx", &red, &green, &blue);
                if(status == 3)ESP_LOGD(TAG_mqtt,"Color : %d,%d,%d",red<<4,green<<4,blue<<4);
            }
            if(!memcmp(event->topic,"/topic/lamp/brightness/value",event->topic_len)){
                memset(buf_brightness,0,SIZE_BUF);
                memcpy(buf_brightness,event->data,event->data_len);
                BRIGHTNESS = atoi(buf_brightness);
                ESP_LOGI(TAG_mqtt, "BRIGHTNESS_value = %d",BRIGHTNESS);
            }
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG_mqtt, "MQTT_EVENT_ERROR");
            break;
        default:
            ESP_LOGI(TAG_mqtt, "Other event id:%d", event->event_id);
            break;
    }
    return ESP_OK;
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    ESP_LOGD(TAG_mqtt, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    mqtt_event_handler_cb(event_data);
}

esp_err_t mqtt_app_start(void)
{
    ESP_LOGI(TAG_mqtt, "Start mqtt");
    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = MQTT_URI,
    };
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_err_t status = esp_mqtt_client_start(client);
    return status;
}
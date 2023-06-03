/* GPIO Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_smartconfig.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "mqtt_client.h"

static EventGroupHandle_t s_wifi_event_group;
static esp_mqtt_client_handle_t client;
static const int CONNECTED_BIT = BIT0;
static const int ESPTOUCH_DONE_BIT = BIT1;
smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();
static const char *TAG = "PJ";
#define device_ID "892cae43-bd95-4a6f-a257-5fba424ab86f"
#define MQTT_Broker "mqtt://mqtt.innoway.vn"

#define ESP_INTR_FLAG_DEFAULT 0

int count = 0;
int pin = 2, status = 0;
static xQueueHandle gpio_evt_queue = NULL;
static xQueueHandle mqttQueue = NULL;
char buffer[100];
TimerHandle_t timer;

void gpio_set(int pin, int status){
    gpio_config_t io_conf;
    io_conf.pin_bit_mask = 1<< pin;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = 0;
    io_conf.pull_down_en = 1;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io_conf);
    gpio_set_level(pin,status);
}

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, 
                                    int32_t event_id, void *event_data)
{
    char *data;
    ESP_LOGI(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        esp_mqtt_client_subscribe(client,"messages/892cae43-bd95-4a6f-a257-5fba424ab86f/status", 0);
        xTimerStart(timer, portMAX_DELAY);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        data = event->data;
        if((strstr(event->topic,"status"))&&(strstr(event->data,"code"))){
            ESP_LOGI(TAG, "code response received");
            xQueueSend(mqttQueue, (void*) data , portMAX_DELAY);
        }
        else if(strstr(event->data, "led"))
        {
            char* token;
            if(strstr(data, "on"))
            {
                status = 1;
            }
            else status = 0;
            token = strtok(data, ":");
            token = strtok(NULL, ",");
            pin = atoi(token);
            ESP_LOGI(TAG, "pin: %d, status: %d", pin, status);
            gpio_set(pin, status);
            xQueueReceive(mqttQueue, buffer, portMAX_DELAY);
        }
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

void mqtt_init(){
    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = MQTT_Broker,
		.password = "6jRccyrI8pPBIPbo0fRyzaSIZIxXII1E",
		.username = "VHT_DEV"
    };
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}

void timer_handler(){
        ESP_LOGI(TAG, "TOPIC heartbeat Publish");
        char mess[100];
        sprintf(mess, "{\"heartbeat\": 1}");
        esp_mqtt_client_publish(client, "messages/892cae43-bd95-4a6f-a257-5fba424ab86f/update",mess, strlen(mess),0,0);
        while(xQueueReceive(mqttQueue, buffer, 20000/portTICK_PERIOD_MS)==0)
        {
            esp_mqtt_client_publish(client, "messages/messages/892cae43-bd95-4a6f-a257-5fba424ab86f/update",
                                                                                    mess, strlen(mess), 0,0);
        }   
}

static void smartconfig_example_task(void * parm);

static void event_handler(void* arg, esp_event_base_t event_base, 
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        xTaskCreate(smartconfig_example_task, "smartconfig_example_task", 4096, NULL, 3, NULL);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
        xEventGroupClearBits(s_wifi_event_group, CONNECTED_BIT);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(s_wifi_event_group, CONNECTED_BIT);
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_SCAN_DONE) {
        ESP_LOGI(TAG, "Scan done");
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_FOUND_CHANNEL) {
        ESP_LOGI(TAG, "Found channel");
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_GOT_SSID_PSWD) {
        ESP_LOGI(TAG, "Got SSID and password");
        smartconfig_event_got_ssid_pswd_t *evt = (smartconfig_event_got_ssid_pswd_t *)event_data;
        wifi_config_t wifi_config;
        uint8_t ssid[33] = { 0 };
        uint8_t password[65] = { 0 };

        bzero(&wifi_config, sizeof(wifi_config_t));
        memcpy(wifi_config.sta.ssid, evt->ssid, sizeof(wifi_config.sta.ssid));
        memcpy(wifi_config.sta.password, evt->password, sizeof(wifi_config.sta.password));
        wifi_config.sta.bssid_set = evt->bssid_set;
        if (wifi_config.sta.bssid_set == true) {
            memcpy(wifi_config.sta.bssid, evt->bssid, sizeof(wifi_config.sta.bssid));
        }

        memcpy(ssid, evt->ssid, sizeof(evt->ssid));
        memcpy(password, evt->password, sizeof(evt->password));
        ESP_LOGI(TAG, "SSID:%s", ssid);
        ESP_LOGI(TAG, "PASSWORD:%s", password);

        ESP_ERROR_CHECK( esp_wifi_disconnect() );
        ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
        esp_wifi_connect();
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_SEND_ACK_DONE) {
        xEventGroupSetBits(s_wifi_event_group, ESPTOUCH_DONE_BIT);
        mqtt_init();
    }
}

static void initialise_wifi(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL) );
    ESP_ERROR_CHECK( esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL) );
    ESP_ERROR_CHECK( esp_event_handler_register(SC_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_start() );
}

static void smartconfig_example_task(void * parm)
{
    EventBits_t uxBits;
    ESP_ERROR_CHECK( esp_smartconfig_set_type(SC_TYPE_ESPTOUCH) );
    ESP_ERROR_CHECK( esp_smartconfig_start(&cfg) );
    while (1) {
        uxBits = xEventGroupWaitBits(s_wifi_event_group, CONNECTED_BIT | ESPTOUCH_DONE_BIT, true, false, portMAX_DELAY); 
        if(uxBits & CONNECTED_BIT) {
            ESP_LOGI(TAG, "WiFi Connected to ap");
        }
        if(uxBits & ESPTOUCH_DONE_BIT) {
            ESP_LOGI(TAG, "smartconfig over");
            esp_smartconfig_stop();
            vTaskDelete(NULL);
        }
    }
}


static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void btn_handle(void* arg)
{
    uint32_t io_num;
    while(1) 
    {
        if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)){
            count++;
            ESP_LOGI(TAG, "Button detect!");
            if(xQueueReceive(gpio_evt_queue, &io_num, 2000/portTICK_PERIOD_MS) == 0)
            {
                if(count == 2){
                    ESP_LOGI(TAG, "led toggle");
                    char mess[100];
                    if(status == 1){
                        sprintf(mess, "{\"led\": %d, \"status\": \"off\"}", pin);
                    }
                    else 
                    {
                        sprintf(mess, "{\"led\": %d, \"status\": \"on\"}", pin);
                    }
                    esp_mqtt_client_publish(client, "messages/892cae43-bd95-4a6f-a257-5fba424ab86f/update",mess, strlen(mess),0,0);
                    while(xQueueReceive(mqttQueue, buffer, 20000/portTICK_PERIOD_MS)==0)
                        {
                        esp_mqtt_client_publish(client, "messages/messages/892cae43-bd95-4a6f-a257-5fba424ab86f/update", 
                                                                                                mess, strlen(mess), 0,0);
                        }  
                }
                else if (count ==4){
                    ESP_LOGI(TAG, "Smart config!");
                    xTimerStop(timer, portMAX_DELAY);
                    esp_mqtt_client_stop(client);
                    esp_wifi_disconnect();
                    esp_wifi_stop();
                    esp_wifi_start();
                }
                else{
                    ESP_LOGI(TAG, "Nothing");
                }
                count = 0;
            }
        }
    }
}

void app_main(void)
{
    nvs_flash_init();
    //zero-initialize the config structure.
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_PIN_INTR_POSEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = 1ULL<<GPIO_NUM_0;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    mqttQueue = xQueueCreate(10, sizeof(uint32_t));
    timer = xTimerCreate("Timer", 10000/portTICK_PERIOD_MS, pdTRUE, (void*) 0, timer_handler);
    xTaskCreate(btn_handle, "gpio_task_example", 3072, NULL, 10, NULL);
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    gpio_isr_handler_add(GPIO_NUM_0, gpio_isr_handler, (void*) GPIO_NUM_0);
    initialise_wifi();
}


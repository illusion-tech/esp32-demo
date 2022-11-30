#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_log.h"
#include "mqtt_client.h"
#include "cJSON.h"
#include "hw507.h"

static const char *TAG = "MQTT_HW507";

#define Aliyun_HOST                     "iot-06z00b3bhjx2czd.mqtt.iothub.aliyuncs.com"
#define Aliyun_PORT                     1883
/** hw507 (温湿度计) 信息 */
#define Aliyun_CLIENT_ID                "hympO86IcSD.hw507|securemode=2,signmethod=hmacsha256,timestamp=1669361558050|"
#define Aliyun_USERNAME                 "hw507&hympO86IcSD"
#define Aliyun_PASSWORD                 "c09f71671b6d65e5de023c1a6478aa8c7f57b9b7323945e6475509d1172df493"
/** topics */
#define AliyunSubscribeTopic_USER_GET   "/hympO86IcSD/hw507/user/get"
#define AliyunPublishTopic_USER_UPDATE  "/hympO86IcSD/hw507/user/update"
#define AliyunSubscribeTopic_POST       "/sys/hympO86IcSD/hw507/thing/event/property/post"
#define AliyunSubscribeTopic_POST_REPLY "/sys/hympO86IcSD/hw507/thing/event/property/post_reply"

esp_mqtt_client_handle_t client;

/** 参数结构体 */
typedef struct
{
    char method;
    char id;
    char version;
    struct
    {
        uint8_t temperature;
        uint8_t Humidity;
    } params;
} mqtt_event_data_t;

static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;

    switch (event->event_id) 
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        // 发布/订阅相关 topic
        msg_id = esp_mqtt_client_publish(client, AliyunPublishTopic_USER_UPDATE, "mqtt connect ok", strlen("mqtt connect ok"), 1, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        msg_id = esp_mqtt_client_subscribe(client, AliyunSubscribeTopic_USER_GET, 0);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        msg_id = esp_mqtt_client_publish(client, AliyunPublishTopic_USER_UPDATE, "mqtt subscribe successful", strlen("mqtt subscribe successful"), 0, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC: %.*s\r\n", event->topic_len, event->topic);
        printf("DATA: %.*s\r\n", event->data_len, event->data);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
    return ESP_OK;
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    mqtt_event_handler_cb(event_data);
}

static void mqtt_test_task(void *pvParameters)
{
    uint8_t num = 0;

    while (1) {
        esp_mqtt_client_publish(client, AliyunPublishTopic_USER_UPDATE, "", 1, 1, 0);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        if (num++ > 2)
            break;
    }
    vTaskDelete(NULL);
}

void iot_hw507_init(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .host = Aliyun_HOST,
        .port = Aliyun_PORT,
        .client_id = Aliyun_CLIENT_ID,
        .username = Aliyun_USERNAME,
        .password = Aliyun_PASSWORD,
    };
    client = esp_mqtt_client_init(&mqtt_cfg);

    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);

    esp_mqtt_client_start(client);

    xTaskCreate(&mqtt_test_task, "mqtt_test_task", 4096, NULL, 5, NULL);

    uint8_t Temp, Humi;
    char mqtt_message[256] = {0};

    while (1) {
        // 读取温湿度信息
        hw507_init();
        Temp = getTemp();
        Humi = getHumi();
        // Humi_small = getHumiSmall();
        // Temp_small = getTempSmall();

        //上传数据格式
        sprintf(
            mqtt_message,
            "{\"method\":\"thing.event.property.post\",\"id\":\"1234567890\",\"params\":{\"temperature\":%d,\"Humidity\":%d},\"version\":\"1.1.1\"}",
            Temp, Humi
        );

        esp_mqtt_client_publish(client, AliyunSubscribeTopic_POST, mqtt_message, strlen(mqtt_message), 0, 0); //上传

        printf("Temp=%d, Humi=%d\r\n", Temp, Humi);

        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

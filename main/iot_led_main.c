#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_log.h"
#include "mqtt_client.h"
#include "cJSON.h"
#include "led_pwm.h"

static const char *TAG = "MQTT_LED_LIGHT";

#define Aliyun_HOST                             "iot-06z00b3bhjx2czd.mqtt.iothub.aliyuncs.com"
#define Aliyun_PORT                             1883
/** led_light (LED 灯) 信息 */
#define Aliyun_CLIENT_ID                        "hympO86IcSD.led_light|securemode=2,signmethod=hmacsha256,timestamp=1669600831109|"
#define Aliyun_USERNAME                         "led_light&hympO86IcSD"
#define Aliyun_PASSWORD                         "5b94fe86e0d8b050fe85f7e0838671def1918ceb4e2ed56a1e9c4504a5695b35"
/** topics */
#define AliyunSubscribeTopic_USER_GET           "/hympO86IcSD/led_light/user/get"
#define AliyunPublishTopic_USER_UPDATE          "/hympO86IcSD/led_light/user/update"
#define AliyunSubscribeTopic_POST               "/sys/hympO86IcSD/led_light/thing/event/property/post"
#define AliyunSubscribeTopic_POST_REPLY         "/sys/hympO86IcSD/led_light/thing/event/property/post_reply"
#define AliyunSubscribeTopic_USER_LIGHT_CONTROL "/hympO86IcSD/led_light/user/light_control"

esp_mqtt_client_handle_t client;

/* light_control topic 返回的参数结构体 */
typedef struct
{
    char method;
    char id;
    char version;
    struct
    {
        uint8_t brightness;
        struct
        {
            uint16_t red;
            uint16_t green;
            uint16_t blue;
        } rgb;
        uint8_t status;
    } params;
} mqtt_event_data_t;

/** light_control topic 接收数据并进行处理 */
static void light_control_topic_event(esp_mqtt_event_handle_t event)
{
    printf("LightData: %.*s\r\n", event->data_len, event->data);
    cJSON *data = cJSON_Parse(event->data);
    cJSON *params = cJSON_GetObjectItem(data, "params");
    cJSON *rgb = cJSON_GetObjectItem(params, "rgb");
    uint16_t red = cJSON_GetObjectItem(rgb, "red")->valueint;
    uint16_t green = cJSON_GetObjectItem(rgb, "green")->valueint;
    uint16_t blue = cJSON_GetObjectItem(rgb, "blue")->valueint;
    uint8_t status = cJSON_GetObjectItem(params, "status")->valueint;
    uint8_t brightness = cJSON_GetObjectItem(params, "brightness")->valueint;

    printf("Status: %d, Red: %d, Green: %d, Blue: %d, Brightness: %d\r\n", status, red, green, blue, brightness);

    if (status) {
        light_driver_set_rgb(red, green, blue);
        light_driver_set_brightness(brightness);
    } else {
        light_driver_set_switch(0);
    }

    cJSON_Delete(data);
}

static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;

    // int event_topic_len = event->topic_len;
    // char event_topic[event_topic_len];      /** 消息体中的 topic */

    switch (event->event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        // 发布/订阅相关 topic
        msg_id = esp_mqtt_client_publish(client, AliyunPublishTopic_USER_UPDATE, "mqtt connect ok", strlen("mqtt connect ok"), 1, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        msg_id = esp_mqtt_client_subscribe(client, AliyunSubscribeTopic_USER_GET, 0);
        msg_id = esp_mqtt_client_subscribe(client, AliyunSubscribeTopic_USER_LIGHT_CONTROL, 0);
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
        // TODO: 截取出 topic
        // strncpy(event_topic, event->topic, event_topic_len);
        // printf("Topic: %.*s\r\n", event_topic);
        // if (strcmp(event_topic, AliyunSubscribeTopic_USER_LIGHT_CONTROL) == 0)
        {
            light_control_topic_event(event);
        }
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

void iot_led_init(void)
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

    ledc_init();
}

/* MQTT (over TCP) Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"
#include "os.h"

#include "driver/gpio.h"
#include "sdkconfig.h"
#include "user_mqtt.h"

#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "hw507.h"
#include "led_pwm.h"

static const char *TAG = "MQTT_EXAMPLE";

#define Aliyun_host "iot-06z00b3bhjx2czd.mqtt.iothub.aliyuncs.com"
#define Aliyun_port 1883
#define Aliyun_client_id "hympO86IcSD.hw507|securemode=2,signmethod=hmacsha256,timestamp=1669361558050|"
#define Aliyun_username "hw507&hympO86IcSD"
#define Aliyun_password "c09f71671b6d65e5de023c1a6478aa8c7f57b9b7323945e6475509d1172df493"
#define AliyunSubscribeTopic_user_get "/hympO86IcSD/hw507/user/get"
#define AliyunPublishTopic_user_update "/hympO86IcSD/hw507/user/update"
#define AliyunSubscribeTopic_post "/sys/hympO86IcSD/hw507/thing/event/property/post"
#define AliyunSubscribeTopic_post_reply "/sys/hympO86IcSD/hw507/thing/event/property/post_reply"
#define AliyunSubscribeTopic_user_light_control "/hympO86IcSD/hw507/user/light_control"


char mqtt_message[256] = {0};
char mqtt_publish_data1[] = "mqtt connect ok ";
char mqtt_publish_data2[] = "mqtt subscribe successful";
char mqtt_publish_data3[] = "mqtt i am esp32";

esp_mqtt_client_handle_t client;

static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
  esp_mqtt_client_handle_t client = event->client;
  int msg_id;

  // your_context_t *context = event->context;
  switch (event->event_id)
  {
  case MQTT_EVENT_CONNECTED:
    ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
    msg_id = esp_mqtt_client_publish(client, AliyunPublishTopic_user_update, mqtt_publish_data1, strlen(mqtt_publish_data1), 1, 0);
    ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

    msg_id = esp_mqtt_client_subscribe(client, AliyunSubscribeTopic_user_get, 0);
    msg_id = esp_mqtt_client_subscribe(client, AliyunSubscribeTopic_user_light_control, 0);
    ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

    break;
  case MQTT_EVENT_DISCONNECTED:
    ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
    break;

  case MQTT_EVENT_SUBSCRIBED:
    ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
    msg_id = esp_mqtt_client_publish(client, AliyunPublishTopic_user_update, mqtt_publish_data2, strlen(mqtt_publish_data2), 0, 0);
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
    if(strcmp(event->topic, AliyunSubscribeTopic_user_light_control) == 0)
    {
      printf("LightDATA=%.*s\r\n", event->data_len, event->data);
    }
    printf("DATA=%.*s\r\n", event->data_len, event->data);
    //todo: 收到订阅消息对led灯处理
    ledc_init();
    blink(LED_RED_CH,3);
    fade(LED_RED_CH);

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

  while (1)
  {
    esp_mqtt_client_publish(client, AliyunPublishTopic_user_update, mqtt_publish_data3, strlen(mqtt_publish_data3), 1, 0);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    if (num++ > 2)
      break;
  }
  vTaskDelete(NULL);
}

/*----------------温湿度--------------*/
// us延时函数，误差不能太大
void DelayUs(uint32_t nCount)
{
  ets_delay_us(nCount);
}

//主函数
void user_mqtt_app_start(void)
{

  esp_mqtt_client_config_t mqtt_cfg = {
      .host = Aliyun_host,
      .port = Aliyun_port,
      .client_id = Aliyun_client_id,
      .username = Aliyun_username,
      .password = Aliyun_password,

  };

  client = esp_mqtt_client_init(&mqtt_cfg);
  esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
  esp_mqtt_client_start(client);
  xTaskCreate(&mqtt_test_task, "mqtt_test_task", 4096, NULL, 5, NULL);
  printf("ESP32 HW507 TEST:%s,%s!\r\n", __DATE__, __TIME__);
  uchar Humi, Humi_small, Temp, Temp_small;
  while (1)
  {
    //读取温湿度信息
    HW507();
    Temp = getTemp();
    Humi = getHumi();
    Humi_small = getHumiSmall();
    Temp_small = getTempSmall();

    //上传数据格式
    sprintf(mqtt_message, "{\"method\":\"thing.event.property.post\",\"id\":\"1234567890\",\"params\":{\"temperature\":%d,\"Humidity\":%d,,\"LEDSwitch\":%d},\"version\":\"1.1.1\"}",
            Temp, Humi,1);

    esp_mqtt_client_publish(client, AliyunSubscribeTopic_post, mqtt_message, strlen(mqtt_message), 0, 0); //上传

    printf("Temp=%d, Humi=%d, LEDSwitch =%d\r\n", Temp, Humi,1);

    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }
}

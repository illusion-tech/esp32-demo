#include <stdio.h>
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "string.h"
#include "hw507.h"
 
 
void app_main()
{
    char hw507_buff[50]={0};
    uchar Humi,Humi_small,Temp,Temp_small;
 
    while(1)
    {
      HW507(); //读取温湿度
      Temp = getTemp();
      Humi = getHumi();
      Humi_small = getHumiSmall();
      Temp_small = getTempSmall();
      printf("Temp=%d.%d℃--Humi=%d.%d%%RH \r\n", Temp,Temp_small,Humi,Humi_small);
      vTaskDelay(300);  //延时300毫秒
    }
}
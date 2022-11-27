#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led_pwm.h"
#include "driver/ledc.h"


void app_main(void)
{
    ledc_init();

    while(1){
        blink(LED_RED_CH,3);
        fade(LED_RED_CH);

        //yello color
        ledc_set_duty_and_update(LEDC_LOW_SPEED_MODE,LED_RED_CH,8191,0);
        ledc_set_duty_and_update(LEDC_LOW_SPEED_MODE,LED_GREEN_CH,8191,0);
        ledc_set_duty_and_update(LEDC_LOW_SPEED_MODE,LED_BLUE_CH,0,0);
        vTaskDelay(2000/portTICK_PERIOD_MS);

        ledc_set_duty_and_update(LEDC_LOW_SPEED_MODE,LED_RED_CH,4095,0);
        ledc_set_duty_and_update(LEDC_LOW_SPEED_MODE,LED_GREEN_CH,4095,0);
        ledc_set_duty_and_update(LEDC_LOW_SPEED_MODE,LED_BLUE_CH,0,0);
        vTaskDelay(2000/portTICK_PERIOD_MS);

        ledc_set_duty_and_update(LEDC_LOW_SPEED_MODE,LED_RED_CH,100,0);
        ledc_set_duty_and_update(LEDC_LOW_SPEED_MODE,LED_GREEN_CH,100,0);
        ledc_set_duty_and_update(LEDC_LOW_SPEED_MODE,LED_BLUE_CH,0,0);
        vTaskDelay(2000/portTICK_PERIOD_MS);

        //close
        //ledc_stop(LEDC_LOW_SPEED_MODE,LED_RED_CH,0);
        //ledc_stop(LEDC_LOW_SPEED_MODE,LED_GREEN_CH,0);
        //ledc_stop(LEDC_LOW_SPEED_MODE,LED_BLUE_CH,0);
        //vTaskDelay(1000/portTICK_PERIOD_MS);

    }
}
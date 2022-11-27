#ifndef _LED_PWM_H_
#define _LED_PWM_H_

#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LED_RED_CH      LEDC_CHANNEL_0
#define LED_GREEN_CH    LEDC_CHANNEL_1
#define LED_BLUE_CH     LEDC_CHANNEL_2

#define LED_RED_PIN     3
#define LED_GREEN_PIN   4
#define LED_BLUE_PIN    5

void ledc_init();

void blink(uint32_t ledc_channel, uint32_t count);
void fade(uint32_t ledc_channel);

#endif

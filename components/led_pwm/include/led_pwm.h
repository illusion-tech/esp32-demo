#ifndef _LED_PWM_H_
#define _LED_PWM_H_

#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LED_RED_CH      LEDC_CHANNEL_0
#define LED_GREEN_CH    LEDC_CHANNEL_1
#define LED_BLUE_CH     LEDC_CHANNEL_2
#define LED_CHANNEL_MAX 3

#define LED_RED_PIN     3
#define LED_GREEN_PIN   4
#define LED_BLUE_PIN    5

#define GAMMA_CORRECTION 0.8                               /**< Gamma curve parameter */
#define GAMMA_TABLE_SIZE 256                               /**< Gamma table size, used for led fade*/

#define LEDC_FADE_MARGIN (10)
#define LEDC_TIMER_PRECISION (LEDC_TIMER_13_BIT)
#define LEDC_VALUE_TO_DUTY(value) (value * ((1 << LEDC_TIMER_PRECISION)) / (UINT16_MAX))
#define LEDC_FIXED_Q (8)
#define FLOATINT_2_FIXED(X, Q) ((int)((X)*(0x1U << Q)))
#define FIXED_2_FLOATING(X, Q) ((int)((X)/(0x1U << Q)))
#define GET_FIXED_INTEGER_PART(X, Q) (X >> Q)
#define GET_FIXED_DECIMAL_PART(X, Q) (X & ((0x1U << Q) - 1))

void ledc_init();

void blink(uint32_t ledc_channel, uint32_t count);

void switch_channel(uint32_t ledc_channel, int status);

esp_err_t light_driver_set_rgb(uint8_t red, uint8_t green, uint8_t blue);

void light_driver_set_switch(int status);

#endif

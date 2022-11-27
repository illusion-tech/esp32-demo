#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "led_pwm.h"

void ledc_init()
{

    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_13_BIT,
        .freq_hz = 5000,
        .clk_cfg = LEDC_AUTO_CLK
    };

    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    ledc_channel_config_t ledc_red_chan = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LED_RED_CH,
        .timer_sel = LEDC_TIMER_0,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = LED_RED_PIN,
        .duty = 0,
        .hpoint = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_red_chan));

    ledc_channel_config_t ledc_green_chan = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LED_GREEN_CH,
        .timer_sel = LEDC_TIMER_0,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = LED_GREEN_PIN,
        .duty = 0,
        .hpoint = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_green_chan));

    ledc_channel_config_t ledc_blue_chan = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LED_BLUE_CH,
        .timer_sel = LEDC_TIMER_0,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = LED_BLUE_PIN,
        .duty = 0,
        .hpoint = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_blue_chan));
    ledc_fade_func_install(0);
}

void blink(uint32_t ledc_channel, uint32_t count)
{
    for (int i = 0; i < count; i++)
    {
        ledc_set_duty_and_update(LEDC_LOW_SPEED_MODE, ledc_channel, 8191, 0);
        vTaskDelay(400 / portTICK_PERIOD_MS);
        ledc_set_duty_and_update(LEDC_LOW_SPEED_MODE, ledc_channel, 0, 0);
        vTaskDelay(400 / portTICK_PERIOD_MS);
    }
}

void fade(uint32_t ledc_channel)
{
    ledc_set_fade_time_and_start(LEDC_LOW_SPEED_MODE, ledc_channel, 8191, 3000, LEDC_FADE_WAIT_DONE);
    ledc_set_fade_time_and_start(LEDC_LOW_SPEED_MODE, ledc_channel, 0, 3000, LEDC_FADE_WAIT_DONE);
}

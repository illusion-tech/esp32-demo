#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "led_pwm.h"

static light_driver_config_t *light_config = NULL;

void led_driver_init(light_driver_config_t *config) {
    if (light_config == NULL) {
        light_config = calloc(1, sizeof(light_driver_config_t));
        light_config->duty = config->duty;
        light_config->speed_mode = config->speed_mode;
        light_config->timer_sel = config->timer_sel;
    }
    register_channel(CHANNEL_ID_RED, config->gpio_red);
    register_channel(CHANNEL_ID_GREEN, config->gpio_green);
    register_channel(CHANNEL_ID_BLUE, config->gpio_blue);
}

void register_channel(ledc_channel_t channel, gpio_num_t gpio_num) {
	const ledc_channel_config_t ledc_ch_config = {
        .gpio_num   = gpio_num,
        .channel    = channel,
        .duty       = light_config->duty,
        .speed_mode = light_config->speed_mode,
        .timer_sel  = light_config->timer_sel,
    };

    ledc_channel_config(&ledc_ch_config);
}

void set_fade(ledc_mode_t speed_mode, ledc_channel_t channel, uint32_t duty) {
	ledc_set_fade_with_time(speed_mode, channel, duty, LEDC_TEST_FADE_TIME);
	ledc_fade_start(speed_mode, channel, LEDC_FADE_NO_WAIT);
}

#ifndef _LED_PWM_H_
#define _LED_PWM_H_

#include "driver/gpio.h"
#include "driver/ledc.h"

#define BLINK_GPIO_R 3
#define BLINK_GPIO_G 4
#define BLINK_GPIO_B 5

// 渐变的变大最终目标占空比
#define LEDC_TEST_DUTY      1000
// 渐变的时间
#define LEDC_TEST_FADE_TIME 2000

enum light_channel {
    CHANNEL_ID_RED = 0,
    CHANNEL_ID_GREEN,
    CHANNEL_ID_BLUE,
};

typedef struct {
    gpio_num_t gpio_red;      /**< Red corresponds to GPIO */
    gpio_num_t gpio_green;    /**< Green corresponds to GPIO */
    gpio_num_t gpio_blue;     /**< Blue corresponds to GPIO */
    ledc_mode_t speed_mode;
    ledc_timer_t timer_sel;
    uint32_t duty;
} light_driver_config_t;

void led_driver_init(light_driver_config_t *config);

void register_channel(ledc_channel_t channel, gpio_num_t gpio_num);

void set_fade(ledc_mode_t speed_mode, ledc_channel_t channel, uint32_t duty);

#endif

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "sdkconfig.h"
#include "led_pwm.h"

void app_main() {
	static ledc_timer_config_t ledc_timer = { 
		.duty_resolution = LEDC_TIMER_10_BIT,    // resolution of PWM duty
		.freq_hz         = 1000,                 // frequency of PWM signal
		.speed_mode      = LEDC_LOW_SPEED_MODE,  // timer mode
		.timer_num       = LEDC_TIMER_0          // timer index
	};
	// Set configuration of timer0 for high speed channels
	ledc_timer_config(&ledc_timer);

	static light_driver_config_t driver_config = {
		.gpio_red   = BLINK_GPIO_R, 
		.gpio_green = BLINK_GPIO_G, 
		.gpio_blue  = BLINK_GPIO_B, 
		.duty       = 0, 
		.speed_mode = LEDC_LOW_SPEED_MODE, 
		.timer_sel  = LEDC_TIMER_0 
	};
	led_driver_init(&driver_config);

	ledc_fade_func_install(0);

	while (1) {
		set_fade(driver_config.speed_mode, CHANNEL_ID_RED, LEDC_TEST_DUTY);
		set_fade(driver_config.speed_mode, CHANNEL_ID_GREEN, LEDC_TEST_DUTY);
		set_fade(driver_config.speed_mode, CHANNEL_ID_BLUE, LEDC_TEST_DUTY);

		vTaskDelay(LEDC_TEST_FADE_TIME / portTICK_PERIOD_MS);

		set_fade(driver_config.speed_mode, CHANNEL_ID_RED, 0);
		set_fade(driver_config.speed_mode, CHANNEL_ID_GREEN, 0);
		set_fade(driver_config.speed_mode, CHANNEL_ID_BLUE, 0);
	
		vTaskDelay(LEDC_TEST_FADE_TIME / portTICK_PERIOD_MS);
	}
}

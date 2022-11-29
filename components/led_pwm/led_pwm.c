#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "errno.h"

#include "math.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "soc/ledc_reg.h"
#include "soc/timer_group_struct.h"
#include "soc/ledc_struct.h"
#include "driver/timer.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "led_pwm.h"

static const char *TAG = "light_driver";
/** 保存 led 的状态 */
static light_state_t *g_light_state = NULL;
static DRAM_ATTR uint16_t *g_gamma_table = NULL;

static void gamma_table_create(uint16_t *gamma_table, float correction)
{
    float value_tmp = 0;

    /**
     * @brief gamma curve formula: y=a*x^(1/gm)
     * x ∈ (0,(GAMMA_TABLE_SIZE-1)/GAMMA_TABLE_SIZE)
     * a = GAMMA_TABLE_SIZE
     */
    for (int i = 0; i < GAMMA_TABLE_SIZE; i++) {
        value_tmp = (float)(i) / (GAMMA_TABLE_SIZE - 1);
        value_tmp = powf(value_tmp, 1.0f / correction);
        gamma_table[i] = (uint16_t)FLOATINT_2_FIXED((value_tmp * GAMMA_TABLE_SIZE), LEDC_FIXED_Q);
    }

    if (gamma_table[255] == 0) {
        gamma_table[255] = __UINT16_MAX__;
    }
}

static IRAM_ATTR uint32_t gamma_value_to_duty(int value)
{
    uint32_t tmp_q = GET_FIXED_INTEGER_PART(value, LEDC_FIXED_Q);
    uint32_t tmp_r = GET_FIXED_DECIMAL_PART(value, LEDC_FIXED_Q);

    uint16_t cur = LEDC_VALUE_TO_DUTY(g_gamma_table[tmp_q]);
    uint16_t next = tmp_q < (GAMMA_TABLE_SIZE - 1) ? LEDC_VALUE_TO_DUTY(g_gamma_table[tmp_q + 1]) : cur;
    uint32_t tmp = (cur + (next - cur) * tmp_r / (0x1U << LEDC_FIXED_Q));
    return tmp;
}

void register_ledc_channel(ledc_channel_t channel,int gpio_num)
{
    ledc_channel_config_t ledc_chan_config = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = channel,
        .timer_sel  = LEDC_TIMER_0,
        .intr_type  = LEDC_INTR_DISABLE,
        .gpio_num   = gpio_num,
        .duty       = 0,
        .hpoint     = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_chan_config));
}

/** led 状态初始化 */
void light_state_init()
{
    g_light_state = calloc(1, sizeof(light_state_t));
    g_light_state->mode            = "RGB";
    g_light_state->status          = 0;
    g_light_state->red             = 255;
    g_light_state->green           = 255;
    g_light_state->blue            = 255;
    g_light_state->brightness      = 100;
    g_light_state->fade_period_ms  = LED_FADE_PERIOD_MS;
    g_light_state->blink_period_ms = LED_BLINK_PERIOD_MS;
}

/* 初始化 */
void ledc_init()
{
    light_state_init();

    ledc_timer_config_t ledc_timer = {
        .speed_mode      = LEDC_LOW_SPEED_MODE,
        .timer_num       = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_13_BIT,
        .freq_hz         = LED_FREQ_HZ,
        .clk_cfg         = LEDC_AUTO_CLK
    };

    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    register_ledc_channel(LED_RED_CH, LED_RED_PIN);
    register_ledc_channel(LED_GREEN_CH, LED_GREEN_PIN);
    register_ledc_channel(LED_BLUE_CH, LED_BLUE_PIN);

    ledc_fade_func_install(0);

    ESP_LOGI(TAG, "led initialized successful");

    if (g_gamma_table == NULL) {
        /* g_gamma_table[GAMMA_TABLE_SIZE] must be 0 */
        g_gamma_table = calloc(GAMMA_TABLE_SIZE + 1, sizeof(uint16_t));
        gamma_table_create(g_gamma_table, GAMMA_CORRECTION);
    } else {
        ESP_LOGE(TAG, "gamma_table has been initialized");
    }
}

/* 指定通道闪烁 */
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

/* 
 * 开启/关闭指定通道
 * status = 1 开启，status = 0 关闭
 */
void switch_channel(uint32_t ledc_channel, int status)
{
    ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE, ledc_channel, status ? 1000 : 0, 1000);
    ledc_fade_start(LEDC_LOW_SPEED_MODE, ledc_channel, LEDC_FADE_WAIT_DONE);
}

/** 配置通道数据 */
static IRAM_ATTR esp_err_t iot_ledc_duty_config(ledc_mode_t speed_mode, ledc_channel_t channel, int hpoint_val, int duty_val,
        uint32_t duty_direction, uint32_t duty_num, uint32_t duty_cycle, uint32_t duty_scale)
{
    if (hpoint_val >= 0) {
        LEDC.channel_group[speed_mode].channel[channel].hpoint.hpoint = hpoint_val & LEDC_HPOINT_LSCH1_V;
    }

    if (duty_val >= 0) {
        LEDC.channel_group[speed_mode].channel[channel].duty.duty = duty_val;
    }

    LEDC.channel_group[speed_mode].channel[channel].conf1.val = ((duty_direction & LEDC_DUTY_INC_LSCH0_V) << LEDC_DUTY_INC_LSCH0_S) |
            ((duty_num & LEDC_DUTY_NUM_LSCH0_V) << LEDC_DUTY_NUM_LSCH0_S) |
            ((duty_cycle & LEDC_DUTY_CYCLE_LSCH0_V) << LEDC_DUTY_CYCLE_LSCH0_S) |
            ((duty_scale & LEDC_DUTY_SCALE_LSCH0_V) << LEDC_DUTY_SCALE_LSCH0_S);

    LEDC.channel_group[speed_mode].channel[channel].conf0.sig_out_en = 1;
    LEDC.channel_group[speed_mode].channel[channel].conf1.duty_start = 1;

    if (speed_mode == LEDC_LOW_SPEED_MODE) {
        LEDC.channel_group[speed_mode].channel[channel].conf0.low_speed_update = 1;
    }

    return ESP_OK;
}

static IRAM_ATTR esp_err_t iot_ledc_set_duty(ledc_mode_t speed_mode, ledc_channel_t channel, uint32_t duty)
{
    return iot_ledc_duty_config(speed_mode,
                                channel,         // uint32_t chan_num,
                                -1,
                                duty << 4,       // uint32_t duty_val,the least 4 bits are decimal part
                                1,               // uint32_t increase,
                                1,               // uint32_t duty_num,
                                1,               // uint32_t duty_cycle,
                                0                // uint32_t duty_scale
                               );
}

static IRAM_ATTR esp_err_t _iot_update_duty(ledc_mode_t speed_mode, ledc_channel_t channel)
{
    LEDC.channel_group[speed_mode].channel[channel].conf0.sig_out_en = 1;
    LEDC.channel_group[speed_mode].channel[channel].conf1.duty_start = 1;

    if (speed_mode == LEDC_LOW_SPEED_MODE) {
        LEDC.channel_group[speed_mode].channel[channel].conf0.low_speed_update = 1;
    }

    return ESP_OK;
}

esp_err_t iot_led_set_channel(ledc_channel_t channel, uint8_t value, uint32_t fade_ms)
{
    int temp_val = FLOATINT_2_FIXED(value, LEDC_FIXED_Q);

    chan_gama_data_t *chan_gama_data = g_light_state->chan_gama_data + channel;

    chan_gama_data->data = temp_val;

    iot_ledc_set_duty(LEDC_LOW_SPEED_MODE, channel, gamma_value_to_duty(temp_val));
    _iot_update_duty(LEDC_LOW_SPEED_MODE, channel);

    return ESP_OK;
}

esp_err_t iot_led_get_channel(ledc_channel_t channel, uint16_t *dst)
{
    int cur = g_light_state->chan_gama_data[channel].data;
    *dst = FIXED_2_FLOATING(cur, LEDC_FIXED_Q);

    return ESP_OK;
}

/* 设置 RGB 格式颜色 */
esp_err_t light_driver_set_rgb(uint16_t red, uint16_t green, uint16_t blue)
{
    g_light_state->mode   = "RGB";
    g_light_state->status = 1;
    g_light_state->red    = red;
    g_light_state->green  = green;
    g_light_state->blue   = blue;

    return ESP_OK;
}

/** 获取 RGB 值 */
esp_err_t light_driver_get_rgb(uint16_t *red, uint16_t *green, uint16_t *blue)
{
    *red   = g_light_state->red;
    *green = g_light_state->green;
    *blue  = g_light_state->blue;

    return ESP_OK;
}

/** 根据亮度计算 RGB 百分比 */
esp_err_t transform_rgb_by_brightness(uint16_t red, uint16_t green, uint16_t blue, uint8_t brightness,
                                    uint16_t *red_val, uint16_t *green_val, uint16_t *blue_val)
{
    *red_val   = brightness * red / 100;
    *green_val = brightness * green / 100;
    *blue_val  = brightness * blue / 100;

    return ESP_OK;
}

/** 设置亮度 */
esp_err_t light_driver_set_brightness(uint8_t brightness)
{
    uint16_t red   = 0;
    uint16_t green = 0;
    uint16_t blue  = 0;
    int32_t fade_period_ms = 0;

    light_driver_get_rgb(&red, &green, &blue);

    transform_rgb_by_brightness(red, green, blue, brightness, &red, &green, &blue);

    // 计算过渡时间
    uint8_t max_color    = MAX(MAX(red, green), blue);
    uint8_t change_value = brightness * 255 / 100 - max_color;
    fade_period_ms       = LED_FADE_PERIOD_MAX_MS * change_value / 255;

    iot_led_set_channel(LED_RED_CH, red, fade_period_ms);
    iot_led_set_channel(LED_GREEN_CH, green, fade_period_ms);
    iot_led_set_channel(LED_BLUE_CH, blue, fade_period_ms);

    g_light_state->brightness = brightness;

    return ESP_OK;
}

/** 获取亮度 */
uint8_t light_driver_get_brightness()
{
    return g_light_state->brightness;
}

/** 开关控制 */
void light_driver_set_switch(int status)
{
    g_light_state->status = status;

    if (status) {
        light_driver_set_rgb(g_light_state->red, g_light_state->green, g_light_state->blue);
        light_driver_set_brightness(g_light_state->brightness);
    } else {
        iot_led_set_channel(LED_RED_CH, 0, 0);
        iot_led_set_channel(LED_GREEN_CH, 0, 0);
        iot_led_set_channel(LED_BLUE_CH, 0, 0);
    }
}

/** 获取开关状态 */
bool light_driver_get_switch()
{
    return g_light_state->status;
}

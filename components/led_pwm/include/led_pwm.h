#ifndef _LED_PWM_H_
#define _LED_PWM_H_

#define LED_RED_CH      LEDC_CHANNEL_0
#define LED_GREEN_CH    LEDC_CHANNEL_1
#define LED_BLUE_CH     LEDC_CHANNEL_2
#define LED_CHANNEL_MAX 3

#define LED_RED_PIN            CONFIG_LED_R_GPIO
#define LED_GREEN_PIN          CONFIG_LED_G_GPIO
#define LED_BLUE_PIN           CONFIG_LED_B_GPIO
#define LED_FADE_PERIOD_MS     100     /**< The time from the current state to the next state */
#define LED_BLINK_PERIOD_MS    1500    /**< Period of blinking lights */
#define LED_FREQ_HZ            5000    /**< frequency of ledc signal */
#define LED_FADE_PERIOD_MAX_MS (3 * 1000)

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

typedef struct {
    int data;
    size_t num;
} chan_gama_data_t;

typedef struct {
    char *mode;
    uint8_t status;           /** 0 | 1 */
    uint16_t red;             /** 0-255 */
    uint16_t green;           /** 0-255 */
    uint16_t blue;            /** 0-255 */
    uint8_t brightness;       /** 0-100 */
    uint32_t fade_period_ms;
    uint32_t blink_period_ms;
    chan_gama_data_t chan_gama_data[LED_CHANNEL_MAX];
} light_state_t;

void ledc_init();

void blink(uint32_t ledc_channel, uint32_t count);

void switch_channel(uint32_t ledc_channel, int status);

esp_err_t light_driver_set_rgb(uint16_t red, uint16_t green, uint16_t blue);
esp_err_t light_driver_get_rgb(uint16_t *red, uint16_t *green, uint16_t *blue);

esp_err_t light_driver_set_brightness(uint8_t brightness);
uint8_t light_driver_get_brightness();

void light_driver_set_switch(int status);
bool light_driver_get_switch();

#endif

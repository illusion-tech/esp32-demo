set(led_pwm_srcs "led_pwm.c")

idf_component_register(SRCS "${led_pwm_srcs}"
                       INCLUDE_DIRS "include")
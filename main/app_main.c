#include <stdio.h>
#include "wifi_station.h"
#include "iot_led_main.h"
#include "iot_hw507_main.h"

void app_main(void)
{
    connect_wifi();

    iot_led_init();

    iot_hw507_init();
}

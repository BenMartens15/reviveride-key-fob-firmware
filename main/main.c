#include <stdio.h>
#include "button.h"
#include "esp_now_core.h"
#include "led.h"

void app_main(void)
{
    esp_now_core_init();
    button_init();
    led_init();
}

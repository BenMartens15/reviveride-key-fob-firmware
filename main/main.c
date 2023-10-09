#include <stdio.h>
#include "esp_now_core.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

send_data_t send_data;

void app_main(void)
{
    esp_now_core_init();

    while(true) {
        esp_now_core_send_data(send_data);
        send_data.start_stop_command = !send_data.start_stop_command;

        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

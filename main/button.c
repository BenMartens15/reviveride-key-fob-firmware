
/* INCLUDES *******************************************************************/
#include "button.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_now_core.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
/******************************************************************************/

/* DEFINES ********************************************************************/
#define ESP_INTR_FLAG_DEFAULT 0

#define BUTTON_PIN 27
#define BUTTON_PIN_MASK (1 << BUTTON_PIN)
#define BUTTON_DEBOUNCE_MS 80
/******************************************************************************/

/* ENUMS **********************************************************************/
/******************************************************************************/

/* STRUCTURES *****************************************************************/
/******************************************************************************/

/* GLOBALS ********************************************************************/
static const char *TAG = "BUTTON";

static SemaphoreHandle_t button_semaphore = NULL;

static engine_state_e engine_state = OFF;
/******************************************************************************/

/* PROTOTYPES *****************************************************************/
static void button_task(void* arg);
/******************************************************************************/

/* PUBLIC FUNCTIONS ***********************************************************/
static void IRAM_ATTR button_isr_handler(void* arg)
{
    xSemaphoreGiveFromISR(button_semaphore, NULL);
}

void button_init(void)
{
    gpio_config_t pin_config = {
        .pin_bit_mask = BUTTON_PIN_MASK,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = 1,
        .intr_type = GPIO_INTR_NEGEDGE
    };

    gpio_config(&pin_config);

    button_semaphore = xSemaphoreCreateBinary();
    xTaskCreate(button_task, "button_task", 2048, NULL, 10, NULL);

    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    gpio_isr_handler_add(BUTTON_PIN, button_isr_handler, NULL);
}
/******************************************************************************/ 

/* PRIVATE FUNCTIONS **********************************************************/
static void button_task(void* arg)
{
    uint16_t button_history = 0b1111111111111111;
    uint8_t check_count = 0;

    while(true) {
        if (xSemaphoreTake(button_semaphore, portMAX_DELAY) == pdTRUE) {
            check_count = 0;
            button_history = 0b1111111111111111;

            while (check_count < BUTTON_DEBOUNCE_MS / 5) {
                button_history = button_history << 1;
                button_history |= gpio_get_level(BUTTON_PIN);

                check_count++;
                vTaskDelay(5 / portTICK_PERIOD_MS);
            }
        }

        if ((button_history & 0b0000000000001111) == 0b0000000000000000) { // checking the last 4 values in the button history
            ESP_LOGI(TAG, "Button pressed");

            engine_state = engine_state ^ 1; // toggle the engine state
            send_data_t send_data = {
                .start_stop_command = engine_state
            };
            esp_now_core_send_data(send_data);
        }
    }
}
/******************************************************************************/

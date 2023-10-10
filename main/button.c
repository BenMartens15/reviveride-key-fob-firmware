
/* INCLUDES *******************************************************************/
#include "button.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_now_core.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "freertos/timers.h"
/******************************************************************************/

/* DEFINES ********************************************************************/
#define ESP_INTR_FLAG_DEFAULT 0

#define BUTTON_PIN 27
#define BUTTON_PIN_MASK (1 << BUTTON_PIN)
#define BUTTON_DEBOUNCE_MS 80

#define TIMER_DIVIDER 16
#define TIMER_SCALE (TIMER_BASE_CLK / TIMER_DIVIDER)  // convert counter value to seconds
/******************************************************************************/

/* ENUMS **********************************************************************/
/******************************************************************************/

/* STRUCTURES *****************************************************************/
/******************************************************************************/

/* GLOBALS ********************************************************************/
static const char *TAG = "BUTTON";

static SemaphoreHandle_t button_semaphore = NULL;
static TimerHandle_t button_timer = NULL;

static engine_state_e engine_state = OFF;
/******************************************************************************/

/* PROTOTYPES *****************************************************************/
static void button_task(void* arg);
static void toggleEngineState(TimerHandle_t xTimer);
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
        .intr_type = GPIO_INTR_ANYEDGE
    };

    gpio_config(&pin_config);

    button_semaphore = xSemaphoreCreateBinary();
    xTaskCreate(button_task, "button_task", 2048, NULL, 10, NULL);

    // the purpose of the button time is to turn the car on or off as soon as the button is held for more than 2 seconds
    button_timer = xTimerCreate("button_hold_timer",
                    pdMS_TO_TICKS(2000),
                    pdFALSE,
                    (void*)0,
                    toggleEngineState);
    // add a check for if button_timer is NULL

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

        if ((button_history & 0b0000000000001111) == 0b0000000000000000) {
            ESP_LOGI(TAG, "Button pressed");
            xTimerStart(button_timer, 0); // start the 2 second button timer
        } else if ((button_history & 0b0000000000001111) == 0b0000000000001111) {
            ESP_LOGI(TAG, "Button released");
            xTimerStop(button_timer, 0);
        }
    }
}

static void toggleEngineState(TimerHandle_t xTimer)
{
    engine_state = engine_state ^ 1;
    send_data_t send_data = {
        .start_stop_command = engine_state
    };
    esp_now_core_send_data(send_data);
}
/******************************************************************************/

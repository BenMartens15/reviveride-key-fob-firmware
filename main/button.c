
/* INCLUDES *******************************************************************/
#include "button.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "esp_log.h"
#include "esp_now_core.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "led.h"
/******************************************************************************/

/* DEFINES ********************************************************************/
#define TAG "BUTTON"
#define BUTTON_LOG_LEVEL ESP_LOG_INFO 

#define ESP_INTR_FLAG_DEFAULT 0

#define BUTTON_PIN 15
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
static SemaphoreHandle_t button_semaphore = NULL;
static TimerHandle_t button_timer = NULL;
/******************************************************************************/

/* PROTOTYPES *****************************************************************/
static void button_task(void* arg);
static void toggle_engine_state(TimerHandle_t xTimer);
/******************************************************************************/

/* PUBLIC FUNCTIONS ***********************************************************/
static void IRAM_ATTR button_isr_handler(void* arg)
{
    xSemaphoreGiveFromISR(button_semaphore, NULL);
}

void button_init(void)
{
    esp_log_level_set(TAG, BUTTON_LOG_LEVEL);

    gpio_config_t pin_config = {
        .pin_bit_mask = BUTTON_PIN_MASK,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = 0,
        .intr_type = GPIO_INTR_NEGEDGE
    };

    ESP_ERROR_CHECK(esp_sleep_enable_ext1_wakeup(BUTTON_PIN_MASK, ESP_EXT1_WAKEUP_ANY_HIGH));

    ESP_LOGI(TAG, "Wakeup cause: %d", esp_sleep_get_wakeup_cause());

    if (gpio_get_level(BUTTON_PIN) == 1) { // if the button is still pressed
        ESP_LOGI(TAG, "Still pressed");

        gpio_config(&pin_config);

        button_semaphore = xSemaphoreCreateBinary();
        xTaskCreate(button_task, "button_task", 2048, NULL, 10, NULL);

        // the purpose of the button time is to turn the car on or off as soon as the button is held for more than 1 seconds (plus ESP boot time)
        button_timer = xTimerCreate("button_hold_timer",
                        pdMS_TO_TICKS(1000),
                        pdFALSE,
                        (void*)0,
                        toggle_engine_state);
        // add a check for if button_timer is NULL
        xTimerStart(button_timer, 0); // start the 2 second button timer

        gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
        gpio_isr_handler_add(BUTTON_PIN, button_isr_handler, NULL);
    } else { // if the button is already released, just go back to sleep
        ESP_LOGI(TAG, "Going to sleep");
        esp_deep_sleep_start();
    }
    
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
            ESP_LOGI(TAG, "Button released");
            xTimerStop(button_timer, 0);
            ESP_LOGI(TAG, "Going to sleep");
            esp_deep_sleep_start(); // go back to sleep if the button is released before 1 second
        }
    }
}

static void toggle_engine_state(TimerHandle_t xTimer)
{
    ESP_LOGI(TAG, "Button held for 1 second");
    gpio_intr_disable(BUTTON_PIN); // disable interrupts on the button pin

    send_data_t send_data = {
        .command = TOGGLE_ENGINE_STATE_COMMAND
    };
    if (esp_now_core_send_data(send_data) == ESP_OK) {
        ESP_LOGI(TAG, "Engine state toggled");
        // flash the LED twice to show that the car has turned on or off
        for (int i = 0; i < 2; i++) {
            led_on();
            vTaskDelay(pdMS_TO_TICKS(1000));
            led_off();
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
        ESP_LOGI(TAG, "Going to sleep");
        esp_deep_sleep_start(); // go back to sleep when finished turning the car on or off
    } else {
        ESP_LOGI(TAG, "Failed to toggle engine state");
        // flash the LED quickly 3 times to show that the data wasn't sent successfully
        for (int i = 0; i < 3; i++) {
            led_on();
            vTaskDelay(pdMS_TO_TICKS(300));
            led_off();
            vTaskDelay(pdMS_TO_TICKS(300));
        }
        ESP_LOGI(TAG, "Going to sleep");
        esp_deep_sleep_start(); // go back to sleep
    }

}
/******************************************************************************/

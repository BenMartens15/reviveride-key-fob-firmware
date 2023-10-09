
/* INCLUDES *******************************************************************/
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_now_core.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include "freertos/event_groups.h"
#include "nvs_flash.h"
/******************************************************************************/

/* DEFINES ********************************************************************/
/******************************************************************************/

/* ENUMS **********************************************************************/
/******************************************************************************/

/* STRUCTURES *****************************************************************/
/******************************************************************************/

/* GLOBALS ********************************************************************/
static const char *TAG = "ESP_NOW_CORE";

static EventGroupHandle_t m_evt_group;
/******************************************************************************/

/* PROTOTYPES *****************************************************************/
static void packet_sent_cb(const uint8_t *mac_addr, esp_now_send_status_t status);
/******************************************************************************/

/* PUBLIC FUNCTIONS ***********************************************************/
void esp_now_core_init(void)
{
    esp_err_t err = ESP_OK;
    const wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    ESP_LOGI(TAG, "Initializing ESP-NOW...");

    m_evt_group = xEventGroupCreate();
    assert(m_evt_group);

    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default() );
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_send_cb(packet_sent_cb));
    ESP_ERROR_CHECK(esp_now_set_pmk((const uint8_t *)ESPNOW_PMK));

    const esp_now_peer_info_t broadcast_destination = {
        .peer_addr = RECEIVER_MAC,
        .channel = ESPNOW_CHANNEL,
        .ifidx = ESP_IF_WIFI_STA
    };
    ESP_ERROR_CHECK(esp_now_add_peer(&broadcast_destination));
    ESP_LOGI(TAG, "ESP-NOW initialized");
}

esp_err_t esp_now_core_send_data(send_data_t data)
{
    esp_err_t err = ESP_OK;
    const uint8_t destination_mac[] = RECEIVER_MAC;

    ESP_LOGI(TAG, "Sending %u bytes to "MACSTR"...", sizeof(data), MAC2STR(destination_mac));
    err = esp_now_send(destination_mac, (uint8_t*)&data, sizeof(data));
    if(err != ESP_OK)
    {
        ESP_LOGE(TAG, "Send error (%d)", err);
        return ESP_FAIL;
    }

        // Wait for callback function to set status bit
    EventBits_t bits = xEventGroupWaitBits(m_evt_group, BIT(ESP_NOW_SEND_SUCCESS) | BIT(ESP_NOW_SEND_FAIL), pdTRUE, pdFALSE, 2000 / portTICK_PERIOD_MS);
    if ( !(bits & BIT(ESP_NOW_SEND_SUCCESS)) )
    {
        if (bits & BIT(ESP_NOW_SEND_FAIL))
        {
            ESP_LOGE(TAG, "Send error");
            return ESP_FAIL;
        }
        ESP_LOGE(TAG, "Send timed out");
        return ESP_ERR_TIMEOUT;
    }

    ESP_LOGI(TAG, "Data sent successfully");
    return ESP_OK;
}
/******************************************************************************/ 

/* PRIVATE FUNCTIONS **********************************************************/
static void packet_sent_cb(const uint8_t *mac_addr, esp_now_send_status_t status) {
    if (mac_addr == NULL) {
        ESP_LOGE(TAG, "Send cb arg error");
        return;
    }

    assert(status == ESP_NOW_SEND_SUCCESS || status == ESP_NOW_SEND_FAIL);
    xEventGroupSetBits(m_evt_group, BIT(status));
}
/******************************************************************************/

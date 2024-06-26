
#ifndef ESP_NOW_H
#define ESP_NOW_H

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include "button.h"
#include "esp_err.h"
/******************************************************************************/

/* DEFINES ********************************************************************/
#define ESPNOW_PMK "g#NX9gxErkNFdEfd"
#define ESPNOW_CHANNEL 1

// command defines - add new commands here
#define TOGGLE_ENGINE_STATE_COMMAND 1

#define RECEIVER_MAC {0x30, 0x30, 0xF9, 0x2A, 0x26, 0xEC} // change to the MAC address of the receiving device
/******************************************************************************/

/* ENUMS **********************************************************************/
/******************************************************************************/

/* STRUCTURES *****************************************************************/
typedef struct {
    uint8_t command;
} send_data_t;
/******************************************************************************/

/* GLOBALS ********************************************************************/
/******************************************************************************/

/* PROTOTYPES *****************************************************************/
void esp_now_core_init(void);
esp_err_t esp_now_core_send_data(send_data_t data);
/******************************************************************************/

#endif /* #ifndef ESP_NOW_H */

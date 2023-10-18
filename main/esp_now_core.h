
#ifndef ESP_NOW_H
#define ESP_NOW_H

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include "button.h"
#include "esp_err.h"
/******************************************************************************/

/* DEFINES ********************************************************************/
#define ESPNOW_PMK "pmk1234567890123"
#define ESPNOW_CHANNEL 1

#define RECEIVER_MAC {0x3C, 0x71, 0xBF, 0xFB, 0xDD, 0x80} // change to the MAC address of the receiving device
/******************************************************************************/

/* ENUMS **********************************************************************/
/******************************************************************************/

/* STRUCTURES *****************************************************************/
typedef struct {
    engine_state_e start_stop_command;
} send_data_t;
/******************************************************************************/

/* GLOBALS ********************************************************************/
/******************************************************************************/

/* PROTOTYPES *****************************************************************/
void esp_now_core_init(void);
esp_err_t esp_now_core_send_data(send_data_t data);
/******************************************************************************/

#endif /* #ifndef ESP_NOW_H */

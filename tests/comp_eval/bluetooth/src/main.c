#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/timers.h"


#include "nvs_flash.h"
#include "esp_err.h"
#include "esp_log.h"

#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "string.h"

#include "stdint.h"
#include "mijaProcl.h"
#include "bleDrv.h"
#include "mijaDevice.h"


void app_main() 
{
	uint8_t btCycle_u8 = 0U;
	printf("BT scan cycle: %d\n\n", btCycle_u8);
	btCycle_u8++;

	mijaDevice_param_t param_st;
	mijaDevice_InitializeParameter(&param_st);
	mijaDevice_Initialize_st(&param_st);
	mijaDevice_Activate_st();

	for(;;);
}




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


EventGroupHandle_t eventGroup_st;


static TimerHandle_t timer_xps;

/* Task to be created. */
void myTask( void * pvParameters )
{
	EventBits_t uxBits_st;
    uint32_t bits_u32 = 8;

	printf("-------------------------- task started...");

    for( ;; )
    {
		uxBits_st = xEventGroupWaitBits(eventGroup_st, bits_u32,
									true, false, portMAX_DELAY); // @suppress("Symbol is not resolved")

		if(0 != (uxBits_st & 8))
        {
			printf("\n timeout received, restart the scan process ");
			xTimerStop(timer_xps, 0);
			//bleDrv_Activate_st();
        }
    }
}

void TimerCallback_vd(TimerHandle_t xTimer)
{
    xEventGroupSetBits(eventGroup_st, 8);
}

/* Function that creates a task. */
void CreateTask( void )
{
	BaseType_t xReturned;
	TaskHandle_t xHandle = NULL;

	eventGroup_st = xEventGroupCreate();

    /* Create the task, storing the handle. */
    xReturned = xTaskCreate(
                    myTask,       /* Function that implements the task. */
                    "NAME",          /* Text name for the task. */
                    4096,   	   /* Stack size in words, not bytes. */
                    ( void * ) 1,    /* Parameter passed into the task. */
                    tskIDLE_PRIORITY,/* Priority at which the task is created. */
                    &xHandle );      /* Used to pass out the created task's handle. */

    if( xReturned == pdPASS )
    {
        /* The task was created.  Use the task's handle to delete the task. */
        //vTaskDelete( xHandle );
		printf("task created...");
    }

	timer_xps = xTimerCreate("Timer", pdMS_TO_TICKS(5000), true,
                                                (void *) 0, TimerCallback_vd);
	xTimerStart(timer_xps, 0);
	
}

void driverCallback_vd(mijaProcl_parsedData_t *data_stp)
{
	if(NULL != data_stp)
	{
		printf("\n ******* SENSOR DATA RESULT **************");
		printf("\n Sensor MAC Address: %02X:", data_stp->macAddr_u8a[0]);
		printf("%02X:", data_stp->macAddr_u8a[1]);
		printf("%02X:", data_stp->macAddr_u8a[2]);
		printf("%02X:", data_stp->macAddr_u8a[3]);
		printf("%02X:", data_stp->macAddr_u8a[4]);
		printf("%02X:", data_stp->macAddr_u8a[5]);
		printf("\n Message UUID %d", data_stp->uuid_u16);
		printf("\n Message counter: %d", data_stp->msgCnt_u8);
		printf("\n Parser result: %d", data_stp->parseResult_u8);
		printf("\n Sensor Data Type: %d", data_stp->dataType_en);
		printf("\n Sensor Data Battery %f (%%)", data_stp->battery_f32);
		printf("\n Sensor Data Temperature: %f (°C)", data_stp->temperature_f32);
		printf("\n Sensor Data Humidity: %f (%%)", data_stp->humidity_f32);
		printf("\n Parser result: %d", data_stp->parseResult_u8);
		printf("\n *****************************************");
	}
}

void app_main() 
{
	uint8_t btCycle_u8 = 0U;
	printf("BT scan cycle: %d\n\n", btCycle_u8);
	btCycle_u8++;

	CreateTask();

	bleDrv_param_t params_st;

	bleDrv_InitializeParameter(&params_st);
	params_st.cycleTimeInSec_u32 = 20;
	params_st.scanDurationInSec_u32 = 20;
	params_st.dataCb_fp = driverCallback_vd;

	bleDrv_Initialize_st(&params_st);

	bleDrv_Activate_st();

	for(;;);

}




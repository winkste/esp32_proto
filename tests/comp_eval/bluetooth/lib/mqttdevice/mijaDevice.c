/****************************************************************************************
* FILENAME :        mijaDevice.c
*
* SHORT DESCRIPTION:
*   This is the mijaDevice handle  
*
* DETAILED DESCRIPTION :     
*
* AUTHOR :    Stephan Wink        CREATED ON :    13. Jan. 2019
*
* Copyright (c) [2020] [Stephan Wink]
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
****************************************************************************************/

/***************************************************************************************/
/* Include Interfaces */

#include "mijaDevice.h"

#include "stdint.h"
#include "stdbool.h"
#include "stddef.h"
#include "string.h"
#include "esp_log.h"
#include "esp_err.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/timers.h"
#include "freertos/queue.h"

#include "mijaProcl.h"
#include "bleDrv.h"

/***************************************************************************************/
/* Local constant defines */
#define TASK_STACK_SIZE     4096
#define QUEUE_ELEMENTS      10

#define TIMER_EVENT         BIT0
#define BLE_DATA_EVENT      BIT1

/***************************************************************************************/
/* Local function like makros */

/***************************************************************************************/
/* Local type definitions (enum, struct, union) */

typedef enum objectState_tag
{
    STATE_NOT_INITIALIZED,
    STATE_INITIALIZED,
    STATE_ACTIVATED,
    STATE_DEACTIVATED
}objectState_t;

typedef struct objectData_tag
{
    objectState_t state_en;
    TaskHandle_t task_xp;
    EventGroupHandle_t eventGroup_st;
    TimerHandle_t timer_xp;
    QueueHandle_t queue_xp;
}objectData_t;
/***************************************************************************************/
/* Local functions prototypes: */
static void Task_vd(void * pvParameters);
static void TimerCallback_vd(TimerHandle_t xTimer);
static void DriverCallback_vd(mijaProcl_parsedData_t *data_stp);
static void HandleQueueEvent_vd(void);

/***************************************************************************************/
/* Local variables: */

static objectData_t this_sst =
{
    .state_en = STATE_NOT_INITIALIZED,
};

/***************************************************************************************/
/* Global functions (unlimited visibility) */

/**--------------------------------------------------------------------------------------
 * @brief     Initializes the initialization structure of the mijaDevice module
*//*-----------------------------------------------------------------------------------*/
esp_err_t mijaDevice_InitializeParameter(mijaDevice_param_t *param_stp)
{
    esp_err_t exeResult_st = ESP_FAIL;

    this_sst.state_en = STATE_NOT_INITIALIZED;

    return(exeResult_st);
}

/**--------------------------------------------------------------------------------------
 * @brief     Initialization of the mijaDevice module
*//*-----------------------------------------------------------------------------------*/
esp_err_t mijaDevice_Initialize_st(mijaDevice_param_t *param_stp)
{
    esp_err_t exeResult_st = ESP_FAIL;
    bleDrv_param_t params_st;

	this_sst.eventGroup_st = xEventGroupCreate();

    this_sst.queue_xp = xQueueCreate(QUEUE_ELEMENTS, sizeof(mijaProcl_parsedData_t));

    (void)xTaskCreate(Task_vd, "MIJATASK", TASK_STACK_SIZE, ( void * ) 1, 
                        tskIDLE_PRIORITY, &this_sst.task_xp ); 

	this_sst.timer_xp = xTimerCreate("Timer", pdMS_TO_TICKS(5000), true,
                                                (void *) 0, TimerCallback_vd);

	bleDrv_InitializeParameter(&params_st);
	params_st.cycleTimeInSec_u32 = 20U;
	params_st.scanDurationInSec_u32 = 20U;
	params_st.dataCb_fp = DriverCallback_vd;

	bleDrv_Initialize_st(&params_st);

    if(    (NULL != this_sst.eventGroup_st) 
        && (NULL != this_sst.timer_xp) 
        && (NULL != this_sst.task_xp)
        && (NULL != this_sst.queue_xp))
    {
        this_sst.state_en = STATE_INITIALIZED;
        exeResult_st = ESP_OK;
    }

    return(exeResult_st);
}

/**--------------------------------------------------------------------------------------
 * @brief     Starts the mijaDevice module
*//*-----------------------------------------------------------------------------------*/
esp_err_t mijaDevice_Activate_st(void)
{
    esp_err_t exeResult_st = ESP_FAIL;

    if(    (STATE_INITIALIZED == this_sst.state_en) 
        || (STATE_DEACTIVATED == this_sst.state_en))
    {
        this_sst.state_en = STATE_ACTIVATED;
        bleDrv_Activate_st();
        exeResult_st = ESP_OK;
    }

    return(exeResult_st);
}

/**--------------------------------------------------------------------------------------
 * @brief     Stops the mijaDevice module
*//*-----------------------------------------------------------------------------------*/
esp_err_t mijaDevice_Deactivate_st(void)
{
    esp_err_t exeResult_st = ESP_FAIL;

    if(    (STATE_INITIALIZED == this_sst.state_en) 
        || (STATE_ACTIVATED == this_sst.state_en))
    {
        this_sst.state_en = STATE_DEACTIVATED;
        bleDrv_Deactivate_st();
        exeResult_st = ESP_OK;
    }

    return(exeResult_st);

}

/***************************************************************************************/
/* Local functions: */

/**--------------------------------------------------------------------------------------
 * @brief     Timer callback to re-trigger the scan process
 * @author    S. Wink
 * @date      17. Jan. 2020
 * @param     xTimer_xp     handle of the timer
 * @return    n/a
*//*-----------------------------------------------------------------------------------*/
static void TimerCallback_vd(TimerHandle_t xTimer_xp)
{
    xEventGroupSetBits(this_sst.eventGroup_st, TIMER_EVENT);
}

/**--------------------------------------------------------------------------------------
 * @brief     Timer callback to re-trigger the scan process
 * @author    S. Wink
 * @date      17. Jan. 2020
 * @param     xTimer_xp     handle of the timer
 * @return    n/a
*//*-----------------------------------------------------------------------------------*/
static void DriverCallback_vd(mijaProcl_parsedData_t *data_stp)
{
	if((NULL != data_stp) && (NULL != this_sst.queue_xp))
    {
        if(pdPASS == xQueueSendToBack(this_sst.queue_xp, (void *) data_stp,
                                            (TickType_t) 10))
        {
            // notify task that new data is available in the queue
            if(NULL != this_sst.eventGroup_st)
            {
                xEventGroupSetBits(this_sst.eventGroup_st, BLE_DATA_EVENT);
            }
        }
    }
}

/**--------------------------------------------------------------------------------------
 * @brief     Handle the queue receive event and print data
 * @author    S. Wink
 * @date      17. Jan. 2020
 * @param     xTimer_xp     handle of the timer
 * @return    n/a
*//*-----------------------------------------------------------------------------------*/
static void HandleQueueEvent_vd(void)
{
    mijaProcl_parsedData_t data_st;

    if(NULL != this_sst.queue_xp)
    {
        // Receive a message on the created queue.  Block for 10 ticks if a
        // message is not immediately available.
        while(pdTRUE == xQueueReceive(this_sst.queue_xp, &data_st, (TickType_t) 10))
        {
            printf("\n ******* SENSOR DATA RESULT **************");
            printf("\n Sensor MAC Address: %02X:", data_st.macAddr_u8a[0]);
            printf("%02X:", data_st.macAddr_u8a[1]);
            printf("%02X:", data_st.macAddr_u8a[2]);
            printf("%02X:", data_st.macAddr_u8a[3]);
            printf("%02X:", data_st.macAddr_u8a[4]);
            printf("%02X", data_st.macAddr_u8a[5]);
            printf("\n Message UUID %d", data_st.uuid_u16);
            printf("\n Message counter: %d", data_st.msgCnt_u8);
            printf("\n Parser result: %d", data_st.parseResult_u8);
            printf("\n Sensor Data Type: %d", data_st.dataType_en);
            printf("\n Sensor Data Battery %f (%%)", data_st.battery_f32);
            printf("\n Sensor Data Temperature: %f (°C)", data_st.temperature_f32);
            printf("\n Sensor Data Humidity: %f (%%)", data_st.humidity_f32);
            printf("\n Parser result: %d", data_st.parseResult_u8);
            printf("\n *****************************************");
        }
    }
}

/**--------------------------------------------------------------------------------------
 * @brief     Task function of the module
 * @author    S. Wink
 * @date      17. Jan. 2020
 * @param     pvParameters_vdp     parameter handle of the task
 * @return    n/a
*//*-----------------------------------------------------------------------------------*/
static void Task_vd(void * pvParameters_vdp)
{
    EventBits_t uxBits_st;
    uint32_t bits_u32 = TIMER_EVENT | BLE_DATA_EVENT;


    for( ;; )
    {
		uxBits_st = xEventGroupWaitBits(this_sst.eventGroup_st, bits_u32,
									true, false, portMAX_DELAY); // @suppress("Symbol is not resolved")

		if(0 != (uxBits_st & BLE_DATA_EVENT))
        {
			HandleQueueEvent_vd();
        }
		if(0 != (uxBits_st & TIMER_EVENT))
        {
			// TODO
			//xTimerStop(this_sst.timer_xp, 0);
        }
    }
}

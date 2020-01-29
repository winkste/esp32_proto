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

/***************************************************************************************/
/* Local constant defines */
#define TASK_STACK_SIZE     4096

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
}objectData_t;
/***************************************************************************************/
/* Local functions prototypes: */
static void Task_vd(void * pvParameters);
static void TimerCallback_vd(TimerHandle_t xTimer);

/***************************************************************************************/
/* Local variables: */

static objectData_t singleton_sst =
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

    singleton_sst.state_en = STATE_NOT_INITIALIZED;

    return(exeResult_st);
}

/**--------------------------------------------------------------------------------------
 * @brief     Initialization of the mijaDevice module
*//*-----------------------------------------------------------------------------------*/
esp_err_t mijaDevice_Initialize_st(mijaDevice_param_t *param_stp)
{
    esp_err_t exeResult_st = ESP_FAIL;

	/*singleton_sst.eventGroup_st = xEventGroupCreate();


    (void)xTaskCreate(Task_vd, "MIJATASK", TASK_STACK_SIZE, ( void * ) 1, 
                        tskIDLE_PRIORITY, &singleton_sst.task_xp ); 

	singleton_sst.timer_xp = xTimerCreate("Timer", pdMS_TO_TICKS(5000), true,
                                                (void *) 0, TimerCallback_vd);

    if(    (NULL != singleton_sst.eventGroup_st) 
        && (NULL != singleton_sst.timer_xp) 
        && (NULL != singleton_sst.timer_xp))
    {
        singleton_sst.state_en = STATE_INITIALIZED;
        exeResult_st = ESP_OK;
    }

    */

    return(exeResult_st);

}

/**--------------------------------------------------------------------------------------
 * @brief     Starts the mijaDevice module
*//*-----------------------------------------------------------------------------------*/
esp_err_t mijaDevice_Activate_st(void)
{
    esp_err_t exeResult_st = ESP_FAIL;

    singleton_sst.state_en = STATE_ACTIVATED;

    return(exeResult_st);

}

/**--------------------------------------------------------------------------------------
 * @brief     Stops the mijaDevice module
*//*-----------------------------------------------------------------------------------*/
esp_err_t mijaDevice_Deactivate_st(void)
{
    esp_err_t exeResult_st = ESP_FAIL;

    singleton_sst.state_en = STATE_DEACTIVATED;

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
    //xEventGroupSetBits(singleton_sst.eventGroup_st, 8);
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
	/*EventBits_t uxBits_st;
    uint32_t bits_u32 = 1 | 2 | 4 | 8;

	printf("-------------------------- task started...");

    for( ;; )
    {
		uxBits_st = xEventGroupWaitBits(singleton_sst.eventGroup_st, bits_u32,
									true, false, portMAX_DELAY); // @suppress("Symbol is not resolved")

        if(0 != (uxBits_st & 1))
        {
			printf("\n---------------------- scan completed received...");
			xTimerStart(singleton_sst.timer_xp, 0);
			
        }

		if(0 != (uxBits_st & 2))
        {
			printf("\n---------------------- scan started received...");
        }

		if(0 != (uxBits_st & 4))
        {
			printf("\n---------------------- sensor parse request received...");
        }
		if(0 != (uxBits_st & 8))
        {
			printf("\n---------------------- timer received...");
			xTimerStop(singleton_sst.timer_xp, 0);
			//esp_ble_gap_start_scanning(10);
        }
    }*/
}



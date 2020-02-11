/****************************************************************************************
* FILENAME :        bleDrv.c
*
* SHORT DESCRIPTION:
*   Header file for bleDrv module.  
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

#include "bleDrv.h"

#include "stdint.h"
#include "stdbool.h"
#include "stddef.h"
#include "string.h"

#include "nvs_flash.h"
#include "esp_err.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/timers.h"

#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"

#include "mijaProcl.h"

/***************************************************************************************/
/* Local constant defines */
#define MAX_SCAN_DURATION   70U
#define MIN_CYCLE_TIME      4U

/***************************************************************************************/
/* Local function like makros */
static bool ParamSetValid_bol(bleDrv_param_t *param_stp);
static void InitializeBleDriver_vd(void);
static void esp_GapCallBack_st(esp_gap_ble_cb_event_t event_en, 
                                    esp_ble_gap_cb_param_t *param_unp);
static void TimerCallback_vd(TimerHandle_t xTimer_xp);

/***************************************************************************************/
/* Local type definitions (enum, struct, union) */

typedef enum objectState_tag
{
    STATE_NOT_INITIALIZED,
    STATE_INITIALIZE_STARTED,
    STATE_READY_FOR_SCAN,
    STATE_SCAN_ACTIVE,
}objectState_t;

typedef struct objectData_tag
{
     objectState_t state_en;
     bleDrv_param_t param_st;
     TimerHandle_t timer_xp;
     bool scanEnabled_bol;
}objectData_t;
/***************************************************************************************/
/* Local functions prototypes: */

/***************************************************************************************/
/* Local variables: */

static objectData_t singleton_sst =
{
        .state_en = STATE_NOT_INITIALIZED,
        .scanEnabled_bol = false,
};

// scan parameters
static esp_ble_scan_params_t bleScanParams_sst = 
{
    .scan_type              = BLE_SCAN_TYPE_PASSIVE,
    .own_addr_type          = BLE_ADDR_TYPE_PUBLIC,
    .scan_filter_policy     = BLE_SCAN_FILTER_ALLOW_ALL,
    .scan_interval          = 0x50,
    .scan_window            = 0x30
};

/***************************************************************************************/
/* Global functions (unlimited visibility) */

/**--------------------------------------------------------------------------------------
 * @brief     Initializes the initialization structure of the bleDrv module
*//*-----------------------------------------------------------------------------------*/
esp_err_t bleDrv_InitializeParameter_st(bleDrv_param_t *param_stp)
{
    esp_err_t exeResult_st = ESP_FAIL;

    if(NULL != param_stp)
    {
        memset(&param_stp, 0U, sizeof(param_stp));
        singleton_sst.state_en = STATE_NOT_INITIALIZED;
        exeResult_st = ESP_OK;
    }

    return(exeResult_st);
}

/**--------------------------------------------------------------------------------------
 * @brief     Initialization of the bleDrv module
*//*-----------------------------------------------------------------------------------*/
esp_err_t bleDrv_Initialize_st(bleDrv_param_t *param_stp)
{
    esp_err_t exeResult_st = ESP_FAIL;
    uint32_t ticks_u32 = 0U;

    if(NULL != param_stp)
    {
        if(    (    (STATE_NOT_INITIALIZED == singleton_sst.state_en) 
                ||  (STATE_READY_FOR_SCAN == singleton_sst.state_en))
            && true == ParamSetValid_bol(param_stp))
        {
            singleton_sst.state_en = STATE_INITIALIZE_STARTED;
            memset(&singleton_sst.param_st, 0U, sizeof(singleton_sst.param_st));
            memcpy(&singleton_sst.param_st, param_stp, sizeof(singleton_sst.param_st));
            InitializeBleDriver_vd();
            ticks_u32 = pdMS_TO_TICKS(singleton_sst.param_st.cycleTimeInSec_u32 * 1000U);
            singleton_sst.timer_xp = xTimerCreate("Timer", ticks_u32, true, (void *) 0, 
                                                    TimerCallback_vd);
            singleton_sst.scanEnabled_bol = false;
            
            if(NULL != singleton_sst.timer_xp)
            {
                xTimerStart(singleton_sst.timer_xp, 0);
                exeResult_st = ESP_OK;
            }
        }
    }

    return(exeResult_st);
}

/**--------------------------------------------------------------------------------------
 * @brief     Starts the bleDrv module
*//*-----------------------------------------------------------------------------------*/
esp_err_t bleDrv_Activate_st(void)
{
    esp_err_t exeResult_st = ESP_FAIL;

    if(STATE_NOT_INITIALIZED != singleton_sst.state_en)
    {
        if(NULL != singleton_sst.timer_xp)
        {
            singleton_sst.scanEnabled_bol = true;
            exeResult_st = ESP_OK;
        }

    }

    return(exeResult_st);
}

/**--------------------------------------------------------------------------------------
 * @brief     Stops the bleDrv module
*//*-----------------------------------------------------------------------------------*/
esp_err_t bleDrv_Deactivate_st(void)
{
    esp_err_t exeResult_st = ESP_FAIL;

    singleton_sst.scanEnabled_bol = false;

    return(exeResult_st);

}

/***************************************************************************************/
/* Local functions: */

/**--------------------------------------------------------------------------------------
 * @brief     Checks the parameter for invalid values
 * @author    S. Wink
 * @date      13. Jan. 2020
 * @param     param_stp     parameter structure pointer
 * @return    true if parameter set is valid and usable, else false
*//*-----------------------------------------------------------------------------------*/
static bool ParamSetValid_bol(bleDrv_param_t *param_stp)
{
    bool exeResult_bol = false;

    if(    (param_stp->scanDurationInSec_u32 < MAX_SCAN_DURATION) 
        && (param_stp->cycleTimeInSec_u32 > MIN_CYCLE_TIME)
        && (NULL != param_stp->dataCb_fp))
    {
        exeResult_bol = true;
    }

    return(exeResult_bol);    
}

/**--------------------------------------------------------------------------------------
 * @brief     CallBack function for bluetooth events from peripherie
 * @author    S. Wink
 * @date      13. Jan. 2020
 * @param     event_stp enumeration of the event source
 * @param     param_unp parameter/results of the event  
 * @return    N/A
*//*-----------------------------------------------------------------------------------*/
static void esp_GapCallBack_st(esp_gap_ble_cb_event_t event_en, 
                                    esp_ble_gap_cb_param_t *param_unp)
{
    mijaProcl_parsedData_t outData_st;

    switch (event_en) 
    {		
		case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: 				
			if(param_unp->scan_param_cmpl.status == ESP_BT_STATUS_SUCCESS) 
            {
                singleton_sst.state_en = STATE_READY_FOR_SCAN;
                //TODO callback initialize is complete				
			}
			else 
            {
                printf("Unable to set scan parameters, error code %d\n\n", 
                                                param_unp->scan_param_cmpl.status);
            }
			break;		
		case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:			
			if(param_unp->scan_start_cmpl.status == ESP_BT_STATUS_SUCCESS) 
			{
				// TODO call back scan is started
                singleton_sst.state_en = STATE_SCAN_ACTIVE;
			}
			else
            { 
                printf("Unable to start scan process, error code %d\n\n", 
                                                param_unp->scan_start_cmpl.status);
            }
			break;		
		case ESP_GAP_BLE_SCAN_RESULT_EVT:
			if(param_unp->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_RES_EVT) 
			{
                if(true == mijaProcl_ParseMessage_bol(&param_unp->scan_rst.ble_adv[0], &outData_st))
                {
                    singleton_sst.param_st.dataCb_fp(&outData_st);
                }
			}
			else if(param_unp->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_CMPL_EVT)
			{
                singleton_sst.state_en = STATE_READY_FOR_SCAN;
				//TODO scan complete callback
			}
			break;		
		default:		
			printf("Event %d unhandled\n\n", event_en);
			break;
	}
}

/**--------------------------------------------------------------------------------------
 * @brief     Initializes the bluetooth modul of the hardware
 * @author    S. Wink
 * @date      13. Jan. 2020
 * @return    N/A
*//*-----------------------------------------------------------------------------------*/
static void InitializeBleDriver_vd(void)
{
    // initialize nvs
	ESP_ERROR_CHECK(nvs_flash_init());
	
	// release memory reserved for classic BT (not used)
	ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
	
	// initialize the BT controller with the default config
	esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
	esp_bt_controller_init(&bt_cfg);
	
	// enable the BT controller in BLE mode
	esp_bt_controller_enable(ESP_BT_MODE_BLE);
	
	// initialize Bluedroid library
	esp_bluedroid_init();
	esp_bluedroid_enable();
	
	// register GAP callback function
	ESP_ERROR_CHECK(esp_ble_gap_register_callback(esp_GapCallBack_st));
	
	// configure scan parameters
	esp_ble_gap_set_scan_params(&bleScanParams_sst);
}

/**--------------------------------------------------------------------------------------
 * @brief     Timer callback to re-trigger the scan process
 * @author    S. Wink
 * @date      17. Jan. 2020
 * @param     xTimer_xp     handle of the timer
 * @return    n/a
*//*-----------------------------------------------------------------------------------*/
static void TimerCallback_vd(TimerHandle_t xTimer_xp)
{
    if(STATE_READY_FOR_SCAN == singleton_sst.state_en)
    {
        if(true == singleton_sst.scanEnabled_bol)
        {
            esp_ble_gap_start_scanning(singleton_sst.param_st.scanDurationInSec_u32);
        }
    }   
}

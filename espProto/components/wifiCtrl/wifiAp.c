/****************************************************************************************
* FILENAME :        wifiAp.c
*
* DESCRIPTION :
*       This module handles the WIFI access point connection
*
* AUTHOR :    Stephan Wink        CREATED ON :    26.08.2019
*
* Copyright (c) [2017] [Stephan Wink]
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
vAUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
****************************************************************************************/

/***************************************************************************************/
/* Include Interfaces */
#include "wifiAp.h"

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_event_legacy.h"
#include "tcpip_adapter.h"

#include "argtable3/argtable3.h"
#include "myConsole.h"
#include "paramif.h"

#include "sdkconfig.h"
#include "wifiIf.h"

/***************************************************************************************/
/* Local constant defines */
#define DEFAULT_WIFI_SSID_AP        "myssid3_"
#define DEFAULT_WIFI_PASS_AP        "testtest"
#define MAXIMUM_AP_CONNECTIONS      2

/***************************************************************************************/
/* Local function like makros */

/***************************************************************************************/
/* Local type definitions (enum, struct, union) */
typedef enum objectState_tag
{
     STATE_NOT_INITIALIZED,
     STATE_INITIALIZED,
     STATE_CONNECTED,
     STATE_DISCONNECTED
}objectState_t;

typedef struct objectData_tag
{
     objectState_t state_en;
     wifiIf_setAp_t params_st;
}objectData_t;

/***************************************************************************************/
/* Local functions prototypes: */
static esp_err_t EventHandler_st(void *ctx_vp, system_event_t *event_stp);

static esp_err_t HandleWifiClientConnected_st(system_event_t *event);
static esp_err_t HandleWifiClientDisConnected_st(system_event_t *event);
static esp_err_t HandleUnexpectedWifiEvent_st(system_event_t *event_stp);

static void ChangeWifiStage_vd(objectState_t state_en);

/***************************************************************************************/
/* Local variables: */
static const char *TAG = "wifiAp";

static EventGroupHandle_t wifiEventGroup_sts;
static objectData_t singleton_sst =
{
        .state_en = STATE_NOT_INITIALIZED
};

/***************************************************************************************/
/* Global functions (unlimited visibility) */

/**--------------------------------------------------------------------------------------
 * @brief     Function to initialize WIFI to station mode
*//*-----------------------------------------------------------------------------------*/
esp_err_t wifiAp_Initialize_st(wifiIf_setAp_t *setup_stp)
{
    esp_err_t exeResult_st = ESP_FAIL;
    wifi_init_config_t cfg_st = WIFI_INIT_CONFIG_DEFAULT();

    if(NULL != setup_stp)
    {
        memcpy(&singleton_sst.params_st, setup_stp, sizeof(singleton_sst.params_st));

        tcpip_adapter_init();
        wifiEventGroup_sts = xEventGroupCreate();
        ESP_ERROR_CHECK(esp_event_loop_init(EventHandler_st, NULL));
        //esp_wifi_deinit();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg_st));
        ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

        ESP_ERROR_CHECK(esp_wifi_stop());
        ESP_ERROR_CHECK(esp_wifi_deinit());
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP,
                                                &setup_stp->wifiSettings_st));
        ESP_ERROR_CHECK(esp_wifi_start());

        ChangeWifiStage_vd(STATE_INITIALIZED);
        ESP_LOGI(TAG, "wifi_init_softap finished.SSID:%s password:%s",
                 DEFAULT_WIFI_SSID_AP, DEFAULT_WIFI_PASS_AP);
        exeResult_st = ESP_OK;
    }
    else
    {
        exeResult_st = ESP_FAIL;
    }

    return(exeResult_st);
}

/***************************************************************************************/
/* Local functions: */

/**--------------------------------------------------------------------------------------
 * @brief     Event handler for WIFI events
 * @author    S. Wink
 * @date      24. Jan. 2019
 * @param     ctx_vp        ???
 * @param     event_stp     the received event
 * @return    a esp_err_t based error code, currently only ESP_OK
*//*-----------------------------------------------------------------------------------*/
static esp_err_t EventHandler_st(void *ctx_vp, system_event_t *event_stp)
{
    esp_err_t result_st = ESP_FAIL;

    ESP_LOGI(TAG, "event handler called");
    switch(event_stp->event_id)
    {
    case SYSTEM_EVENT_AP_STACONNECTED:
        result_st = HandleWifiClientConnected_st(event_stp);
        break;
    case SYSTEM_EVENT_AP_STADISCONNECTED:
        result_st = HandleWifiClientDisConnected_st(event_stp);
        break;
    default:
        result_st = HandleUnexpectedWifiEvent_st(event_stp);
        break;
    }

    return(result_st);
}

/**--------------------------------------------------------------------------------------
 * @brief     Event function for wifi client connected to our access point
 * @author    S. Wink
 * @date      25. Jul. 2019
 * @param     event     the received event
 * @return    a esp_err_t based error code, currently only ESP_OK
*//*-----------------------------------------------------------------------------------*/
static esp_err_t HandleWifiClientConnected_st(system_event_t *event)
{
    ESP_LOGI(TAG, "SYSTEM_EVENT_AP_STACONNECTED event received...");

    ESP_LOGI(TAG, "station2:"MACSTR" join, AID=%d",
             MAC2STR(event->event_info.sta_connected.mac),
             event->event_info.sta_connected.aid);
    if(NULL != singleton_sst.params_st.wifiCallBacks_st.callBackClientConnected_fp)
    {
        (*singleton_sst.params_st.wifiCallBacks_st.callBackClientConnected_fp)();
        ESP_LOGI(TAG, "executed wifi ap connect callback function...");
    }
    ChangeWifiStage_vd(STATE_CONNECTED);

    return(ESP_OK);
}

/**--------------------------------------------------------------------------------------
 * @brief     Event function for wifi client disconnected from our access point
 * @author    S. Wink
 * @date      25. Jul. 2019
 * @param     event     the received event
 * @return    a esp_err_t based error code, currently only ESP_OK
*//*-----------------------------------------------------------------------------------*/
static esp_err_t HandleWifiClientDisConnected_st(system_event_t *event)
{
    ESP_LOGI(TAG, "SYSTEM_EVENT_AP_STADISCONNECTED event received...");

    ESP_LOGI(TAG, "station:"MACSTR"leave, AID=%d",
             MAC2STR(event->event_info.sta_disconnected.mac),
             event->event_info.sta_disconnected.aid);
    if(NULL != singleton_sst.params_st.wifiCallBacks_st.callBackClientDisconn_fp)
    {
        (*singleton_sst.params_st.wifiCallBacks_st.callBackClientDisconn_fp)();
        ESP_LOGI(TAG, "executed wifi ap disconnect callback function...");
    }
    ChangeWifiStage_vd(STATE_DISCONNECTED);

    return(ESP_OK);
}

/**--------------------------------------------------------------------------------------
 * @brief     Event function for unexpected wifi event
 * @author    S. Wink
 * @date      25. Jul. 2019
 * @param     event_stp   the received event
 * @return    a esp_err_t based error code, currently only ESP_OK
*//*-----------------------------------------------------------------------------------*/
static esp_err_t HandleUnexpectedWifiEvent_st(system_event_t *event_stp)
{
    ESP_LOGE(TAG, "unexpected wifi event received: %d", event_stp->event_id);

    return(ESP_OK);
}

/**--------------------------------------------------------------------------------------
 * @brief     Change the state of the internal object based on the wifi state
 * @author    S. Wink
 * @date      28. Aug. 2019
 * @param     state_en     the new state
 * @return    -
*//*-----------------------------------------------------------------------------------*/
static void ChangeWifiStage_vd(objectState_t state_en)
{
    ESP_LOGI(TAG, "wifi state changed: %d", state_en);
    singleton_sst.state_en = state_en;
}

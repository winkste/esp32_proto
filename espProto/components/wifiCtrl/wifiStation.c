/****************************************************************************************
* FILENAME :        wifiStation.c
*
* DESCRIPTION :
*       This module handles the WIFI station connection
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

#include "wifiStation.h"

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
#include "utils.h"

#include "sdkconfig.h"
#include "wifiIf.h"

/***************************************************************************************/
/* Local constant defines */
#define MAXIMUM_NUM_OF_RETRIES      CONFIG_ESP_MAXIMUM_RETRY
#define MAXIMUM_CONN_RETRIES        2

/***************************************************************************************/
/* Local function like makros */

/***************************************************************************************/
/* Local type definitions (enum, struct, union) */
typedef enum objectState_tag
{
     STATE_NOT_INITIALIZED,
     STATE_INITIALIZED,
     STATE_CONNECT_STARTED,
     STATE_CONNECT_IN_PROGRESS,
     STATE_CONNECTED,
     STATE_RECOVER_FROM_DISCONN,
     STATE_DISCONNECTED
}objectState_t;

typedef struct objectData_tag
{
     objectState_t state_en;
     wifiIf_setStation_t params_st;
}objectData_t;

/***************************************************************************************/
/* Local functions prototypes: */
static esp_err_t EventHandler_st(void *ctx_vp, system_event_t *event_stp);

static esp_err_t HandleStationStartRequestEvent_st(system_event_t *event);
static esp_err_t HandleStationConnectedEvent_st(system_event_t *event);
static esp_err_t HandleStationGotIpEvent_st(system_event_t *event);
static esp_err_t HandleStationGotIp6Event_st(system_event_t *event);
static esp_err_t HandleWifiStationDisconnect_st(system_event_t *event);
static esp_err_t HandleUnexpectedWifiEvent_st(system_event_t *event);

static void ChangeWifiStage_vd(objectState_t state_en);

/***************************************************************************************/
/* Local variables: */
static const char *TAG = "wifiStation";

static EventGroupHandle_t wifiEventGroup_sts;
static uint8_t retryConnectCounter_u8st = 0U;

static objectData_t singleton_sst =
{
        .state_en = STATE_NOT_INITIALIZED,
};

/***************************************************************************************/
/* Global functions (unlimited visibility) */

/**--------------------------------------------------------------------------------------
 * @brief     Function to initialize WIFI to station mode
*//*-----------------------------------------------------------------------------------*/
esp_err_t wifiStation_Initialize_st(wifiIf_setStation_t *setup_stp)
{
    esp_err_t exeResult_st = ESP_FAIL;
    wifi_init_config_t cfg_st = WIFI_INIT_CONFIG_DEFAULT();

    if(NULL != setup_stp)
    {
        memcpy(&singleton_sst.params_st, setup_stp, sizeof(singleton_sst.params_st));

        tcpip_adapter_init();
        wifiEventGroup_sts = xEventGroupCreate();
        utils_CheckAndLogExecution_bol(TAG, esp_event_loop_init(EventHandler_st, NULL), __LINE__);
        //esp_wifi_deinit();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg_st));
        ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
        ESP_ERROR_CHECK(esp_wifi_stop());
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA,
                                                &setup_stp->wifiSettings_st));
        ESP_ERROR_CHECK(esp_wifi_start());

        ChangeWifiStage_vd(STATE_INITIALIZED);
        ESP_LOGI(TAG, "connection started to access point SSID:%s password:%s",
                &singleton_sst.params_st.wifiSettings_st.sta.ssid[0],
                &singleton_sst.params_st.wifiSettings_st.sta.password[0]);
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
    case SYSTEM_EVENT_STA_START:
        result_st = HandleStationStartRequestEvent_st(event_stp);
        break;
    case SYSTEM_EVENT_STA_CONNECTED:
        result_st = HandleStationConnectedEvent_st(event_stp);
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        result_st = HandleStationGotIpEvent_st(event_stp);
        break;
    case SYSTEM_EVENT_AP_STA_GOT_IP6:
        result_st = HandleStationGotIp6Event_st(event_stp);
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        result_st = HandleWifiStationDisconnect_st(event_stp);
        break;
    case SYSTEM_EVENT_AP_STACONNECTED:
    case SYSTEM_EVENT_AP_STADISCONNECTED:
    default:
        result_st = HandleUnexpectedWifiEvent_st(event_stp);
        break;
    }

    return(result_st);
}

/**--------------------------------------------------------------------------------------
 * @brief     Event function for wifi station connection
 * @author    S. Wink
 * @date      25. Jul. 2019
 * @param     event     the received event
 * @return    a esp_err_t based error code, currently only ESP_OK
*//*-----------------------------------------------------------------------------------*/
static esp_err_t HandleStationStartRequestEvent_st(system_event_t *event)
{
    ESP_LOGI(TAG, "SYSTEM_EVENT_STA_START event received...");
    esp_wifi_connect();

    ChangeWifiStage_vd(STATE_CONNECT_STARTED);
    /*if(NULL != singleton_sst.params_st.wifiCallBacks_st.callBackStationConStarted_ptrs)
    {
        (*singleton_sst.params_st.wifiCallBacks_st.callBackStationConStarted_ptrs)();
        ESP_LOGI(TAG, "callBack function for wifi start executed...");
    }*/

    return(ESP_OK);
}

/**--------------------------------------------------------------------------------------
 * @brief     Event function for wifi station connected event
 * @author    S. Wink
 * @date      25. Jul. 2019
 * @param     event     the received event
 * @return    a esp_err_t based error code, currently only ESP_OK
*//*-----------------------------------------------------------------------------------*/
static esp_err_t HandleStationConnectedEvent_st(system_event_t *event)
{
    ESP_LOGI(TAG, "SYSTEM_EVENT_STA_CONNECTED event received...");
    // enable ipv6
    tcpip_adapter_create_ip6_linklocal(TCPIP_ADAPTER_IF_STA);

    ChangeWifiStage_vd(STATE_CONNECT_IN_PROGRESS);

    return(ESP_OK);
}

/**--------------------------------------------------------------------------------------
 * @brief     Event function for wifi station successful got ip
 * @author    S. Wink
 * @date      25. Jul. 2019
 * @param     event     the received event
 * @return    a esp_err_t based error code, currently only ESP_OK
*//*-----------------------------------------------------------------------------------*/
static esp_err_t HandleStationGotIpEvent_st(system_event_t *event)
{
    ESP_LOGI(TAG, "SYSTEM_EVENT_STA_GOT_IP event received...");
    return(ESP_OK);
}

/**--------------------------------------------------------------------------------------
 * @brief     Event function for wifi station successful got ip 6
 * @author    S. Wink
 * @date      25. Jul. 2019
 * @param     event     the received event
 * @return    a esp_err_t based error code, currently only ESP_OK
*//*-----------------------------------------------------------------------------------*/
static esp_err_t HandleStationGotIp6Event_st(system_event_t *event)
{
    char *ip6 = ip6addr_ntoa(&event->event_info.got_ip6.ip6_info.ip); // @suppress("Field cannot be resolved")

    ESP_LOGI(TAG, "SYSTEM_EVENT_STA_GOT_IP6 event received...");
    ESP_LOGI(TAG, "IPv6: %s", ip6);

    ESP_LOGI(TAG, "wifi connection successful.");
    ChangeWifiStage_vd(STATE_CONNECTED);

    if(NULL != singleton_sst.params_st.wifiCallBacks_st.callBackStationConnected_fp)
    {
        (*singleton_sst.params_st.wifiCallBacks_st.callBackStationConnected_fp)();
        ESP_LOGI(TAG, "callBack function for wifi start executed...");
    }

    return(ESP_OK);
}

/**--------------------------------------------------------------------------------------
 * @brief     Event function for wifi disconnection
 * @author    S. Wink
 * @date      25. Jul. 2019
 * @param     event     the received event
 * @return    a esp_err_t based error code, currently only ESP_OK
*//*-----------------------------------------------------------------------------------*/
static esp_err_t HandleWifiStationDisconnect_st(system_event_t *event)
{
    ESP_LOGI(TAG, "SYSTEM_EVENT_STA_DISCONNECTED event received...");

    if(MAXIMUM_CONN_RETRIES > retryConnectCounter_u8st)
    {
        if(NULL != singleton_sst.params_st.wifiCallBacks_st.callBackStationDisconn_fp)
        {
            (*singleton_sst.params_st.wifiCallBacks_st.callBackStationDisconn_fp)();
        }
        /* This is a workaround as ESP32 WiFi libs don't currently
        auto-reassociate.*/
        esp_wifi_connect();
        ESP_LOGI(TAG,"retry %d to connect to the AP",
                            retryConnectCounter_u8st);
        retryConnectCounter_u8st++;
        ChangeWifiStage_vd(STATE_RECOVER_FROM_DISCONN);
    }
    else
    {
        retryConnectCounter_u8st = 0U;
        ChangeWifiStage_vd(STATE_DISCONNECTED);
    }

    return(ESP_OK);
}

/**--------------------------------------------------------------------------------------
 * @brief     Event function for unexpected wifi event
 * @author    S. Wink
 * @date      25. Jul. 2019
 * @param     event     the received event
 * @return    a esp_err_t based error code, currently only ESP_OK
*//*-----------------------------------------------------------------------------------*/
static esp_err_t HandleUnexpectedWifiEvent_st(system_event_t *event)
{
    ESP_LOGE(TAG, "unexpected wifi event received: %d", event->event_id);

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

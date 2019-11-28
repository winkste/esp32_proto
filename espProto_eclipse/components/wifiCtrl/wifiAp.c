/****************************************************************************************
* FILENAME :        wifiAp.c
*
* DESCRIPTION :
*       This module handles the WIFI access point connection.
*
* AUTHOR :    Stephan Wink        CREATED ON :    15.09.2019
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
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event_legacy.h"
#include "tcpip_adapter.h"

#include "utils.h"
#include "sdkconfig.h"
#include "wifiIf.h"


/***************************************************************************************/
/* Local constant defines */
#define DEFAULT_WIFI_SSID_AP        "myssid3_"
#define DEFAULT_WIFI_PASS_AP        "testtest"
#define MAXIMUM_AP_CONNECTIONS      2
#define MODULE_TAG                  "wifiAp"

#define CHECK_EXE(arg) utils_CheckAndLogExecution_bol(MODULE_TAG, arg, __LINE__)

/***************************************************************************************/
/* Local function like makros */

/***************************************************************************************/
/* Local type definitions (enum, struct, union) */

/***************************************************************************************/
/* Local functions prototypes: */

/***************************************************************************************/
/* Local variables: */
static const char *TAG = MODULE_TAG;

static const wifiIf_eventLookup_t events_ssta[SYSTEM_EVENT_MAX] =
{
    {SYSTEM_EVENT_WIFI_READY,           wifiIf_EVENT_DONT_CARE},
    {SYSTEM_EVENT_SCAN_DONE,            wifiIf_EVENT_DONT_CARE},
    {SYSTEM_EVENT_STA_START,            wifiIf_EVENT_DONT_CARE},
    {SYSTEM_EVENT_STA_STOP,             wifiIf_EVENT_DONT_CARE},
    {SYSTEM_EVENT_STA_CONNECTED,        wifiIf_EVENT_DONT_CARE},
    {SYSTEM_EVENT_STA_DISCONNECTED,     wifiIf_EVENT_DONT_CARE},
    {SYSTEM_EVENT_STA_AUTHMODE_CHANGE,  wifiIf_EVENT_DONT_CARE},
    {SYSTEM_EVENT_STA_GOT_IP,           wifiIf_EVENT_DONT_CARE},
    {SYSTEM_EVENT_STA_LOST_IP,          wifiIf_EVENT_DONT_CARE},
    {SYSTEM_EVENT_STA_WPS_ER_SUCCESS,   wifiIf_EVENT_DONT_CARE},
    {SYSTEM_EVENT_STA_WPS_ER_FAILED,    wifiIf_EVENT_DONT_CARE},
    {SYSTEM_EVENT_STA_WPS_ER_TIMEOUT,   wifiIf_EVENT_DONT_CARE},
    {SYSTEM_EVENT_STA_WPS_ER_PIN,       wifiIf_EVENT_DONT_CARE},
    {SYSTEM_EVENT_AP_START,             wifiIf_EVENT_DONT_CARE},
    {SYSTEM_EVENT_AP_STOP,              wifiIf_EVENT_DONT_CARE},
    {SYSTEM_EVENT_AP_STACONNECTED,      wifiIf_EVENT_DONT_CARE},
    {SYSTEM_EVENT_AP_STADISCONNECTED,   wifiIf_EVENT_CLIENT_DISCONNECTED},
    {SYSTEM_EVENT_AP_STAIPASSIGNED,     wifiIf_EVENT_CLIENT_CONNECTED},
    {SYSTEM_EVENT_AP_PROBEREQRECVED,    wifiIf_EVENT_DONT_CARE},
    {SYSTEM_EVENT_GOT_IP6,              wifiIf_EVENT_DONT_CARE},
    {SYSTEM_EVENT_ETH_START,            wifiIf_EVENT_DONT_CARE},
    {SYSTEM_EVENT_ETH_STOP,             wifiIf_EVENT_DONT_CARE},
    {SYSTEM_EVENT_ETH_CONNECTED,        wifiIf_EVENT_DONT_CARE},
    {SYSTEM_EVENT_ETH_DISCONNECTED,     wifiIf_EVENT_DONT_CARE},
    {SYSTEM_EVENT_ETH_GOT_IP,           wifiIf_EVENT_DONT_CARE}
};

static wifi_config_t apWifiSettings_sts =
{
    .ap =
    {
        .ssid = DEFAULT_WIFI_SSID_AP,
        .ssid_len = strlen(DEFAULT_WIFI_SSID_AP),
        .password = DEFAULT_WIFI_PASS_AP,
        .max_connection = MAXIMUM_AP_CONNECTIONS,
        .authmode = WIFI_AUTH_WPA_WPA2_PSK
    },
};

/***************************************************************************************/
/* Global functions (unlimited visibility) */

/**---------------------------------------------------------------------------------------
 * @brief     Function to start the wifi controlling
*//*-----------------------------------------------------------------------------------*/
esp_err_t wifiAp_Start_st(void)
{
    esp_err_t result_st = ESP_OK;
    bool exeResult_bol = true;
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    tcpip_adapter_init();
    exeResult_bol &= CHECK_EXE(esp_wifi_init(&cfg));
    exeResult_bol &= CHECK_EXE(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    exeResult_bol &= CHECK_EXE(esp_wifi_set_mode(WIFI_MODE_AP));
    exeResult_bol &= CHECK_EXE(esp_wifi_set_config(ESP_IF_WIFI_AP, &apWifiSettings_sts));
    exeResult_bol &= CHECK_EXE(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished.SSID:%s password:%s",
             DEFAULT_WIFI_SSID_AP, DEFAULT_WIFI_PASS_AP);

    if(false == exeResult_bol)
    {
        result_st = ESP_FAIL;
    }
    return(result_st);
}

/**---------------------------------------------------------------------------------------
 * @brief     Function to stop the wifi
*//*-----------------------------------------------------------------------------------*/
esp_err_t wifiAp_Stop_st(void)
{
    esp_err_t result_st = ESP_FAIL;
    bool exeResult_bol = true;

    // questionable: tcpip_adapter_down(TCPIP_ADAPTER_IF_STA);
    exeResult_bol &= CHECK_EXE(esp_wifi_disconnect());
    exeResult_bol &= CHECK_EXE(esp_wifi_stop());
    exeResult_bol &= CHECK_EXE(esp_wifi_deinit());

    if(true == exeResult_bol)
    {
        result_st = ESP_OK;
    }

    return(result_st);
}

/**--------------------------------------------------------------------------------------
 * @brief     Event converter
 * @author    S. Wink
 * @date      15. Sep. 2019
 * @param     event_stp     the received event
 * @return    converted event id fitting to the wifi module
*//*-----------------------------------------------------------------------------------*/
uint32_t wifiAp_EventConverter_u32(system_event_t *event_stp)
{
    uint32_t event_u32 = wifiIf_EVENT_DONT_CARE;
    uint8_t index_u8 = 0U;

    while(index_u8 < sizeof(events_ssta))
    {
        if(event_stp->event_id == events_ssta[index_u8].eventId_st)
        {
            event_u32 = events_ssta[index_u8].wifiEvent_u32;
            ESP_LOGI(TAG, "received event %d matches wifi event %d in table index %d",
                                event_stp->event_id, event_u32, index_u8);
            break;
        }
        index_u8++;
    }

    return(event_u32);
}

/***************************************************************************************/
/* Local functions: */

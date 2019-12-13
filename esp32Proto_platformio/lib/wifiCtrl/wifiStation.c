/****************************************************************************************
* FILENAME :        wifiStation.c
*
* DESCRIPTION :
*       This module handles the WIFI station connection.
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
#include "wifiStation.h"

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event_legacy.h"
#include "tcpip_adapter.h"

#include "utils.h"
#include "wifiIf.h"


/***************************************************************************************/
/* Local constant defines */
#define MODULE_TAG                  "wifiStation"

#define CHECK_EXE(arg) utils_CheckAndLogExecution_bol(MODULE_TAG, arg, __LINE__)

/***************************************************************************************/
/* Local function like makros */

static void ConnectWithStation_vd(void);
static void CreateIpLink_vd(void);
static void PrintReceivedIp_vd(system_event_t *event);

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
    {SYSTEM_EVENT_AP_STADISCONNECTED,   wifiIf_EVENT_DONT_CARE},
    {SYSTEM_EVENT_AP_STAIPASSIGNED,     wifiIf_EVENT_DONT_CARE},
    {SYSTEM_EVENT_AP_PROBEREQRECVED,    wifiIf_EVENT_DONT_CARE},
    {SYSTEM_EVENT_GOT_IP6,              wifiIf_EVENT_STATION_CONNECTED},
    {SYSTEM_EVENT_ETH_START,            wifiIf_EVENT_DONT_CARE},
    {SYSTEM_EVENT_ETH_STOP,             wifiIf_EVENT_DONT_CARE},
    {SYSTEM_EVENT_ETH_CONNECTED,        wifiIf_EVENT_DONT_CARE},
    {SYSTEM_EVENT_ETH_DISCONNECTED,     wifiIf_EVENT_STATION_DISCONNECTED},
    {SYSTEM_EVENT_ETH_GOT_IP,           wifiIf_EVENT_DONT_CARE}
};

static wifi_config_t stationWifiSettings_sts =
{
    .sta =
    {
        .ssid = wifiIf_DEFAULT_WIFI_SSID_STA,
        .password = wifiIf_DEFAULT_WIFI_PASS_STA
    },
};

/***************************************************************************************/
/* Global functions (unlimited visibility) */

/**---------------------------------------------------------------------------------------
 * @brief     Function to start the wifi controlling
*//*-----------------------------------------------------------------------------------*/
esp_err_t wifiStation_Start_st(wifiIf_stationParam_t *params_stp)
{
    esp_err_t result_st = ESP_FAIL;
    bool exeResult_bol = true;
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    tcpip_adapter_init();
    exeResult_bol &= CHECK_EXE(esp_wifi_init(&cfg));
    exeResult_bol &= CHECK_EXE(esp_wifi_set_storage(WIFI_STORAGE_RAM));

    memset(stationWifiSettings_sts.sta.ssid, 0,
            sizeof(stationWifiSettings_sts.sta.ssid));
    memcpy(&stationWifiSettings_sts.sta.ssid[0],
            &params_stp->ssid[0],
            sizeof(stationWifiSettings_sts.sta.ssid));
    memset(stationWifiSettings_sts.sta.password, 0,
            sizeof(stationWifiSettings_sts.sta.password));
    memcpy(&stationWifiSettings_sts.sta.password[0],
            &params_stp->password[0],
            sizeof(stationWifiSettings_sts.sta.password));

    exeResult_bol &= CHECK_EXE(esp_wifi_stop());

    exeResult_bol &= CHECK_EXE(esp_wifi_set_mode(WIFI_MODE_STA));
    exeResult_bol &= CHECK_EXE(esp_wifi_set_config(ESP_IF_WIFI_STA,
                                        &stationWifiSettings_sts));
    exeResult_bol &= CHECK_EXE(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished.");
        ESP_LOGI(TAG, "connect to ap SSID:%s password:%s",
                &params_stp->ssid[0], &params_stp->password[0]);

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
uint32_t wifiStation_EventConverter_u32(system_event_t *event_stp)
{
    uint32_t event_u32 = wifiIf_EVENT_DONT_CARE;
    uint8_t index_u8 = 0U;

    // do internal event processing first
    switch(event_stp->event_id)
    {
        case SYSTEM_EVENT_STA_START:
            ConnectWithStation_vd();
            break;
        case SYSTEM_EVENT_STA_CONNECTED:
            CreateIpLink_vd();
            break;
        case SYSTEM_EVENT_AP_STA_GOT_IP6:
            PrintReceivedIp_vd(event_stp);
            break;
        default:
            break;
    }

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

/**---------------------------------------------------------------------------------------
 * @brief     Function to stop the wifi
*//*-----------------------------------------------------------------------------------*/
esp_err_t wifiStation_Stop_st(void)
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

/***************************************************************************************/
/* Local functions: */

/**--------------------------------------------------------------------------------------
 * @brief     Starts to connect to the station
 * @author    S. Wink
 * @date      20. Sep. 2019
*//*-----------------------------------------------------------------------------------*/
static void ConnectWithStation_vd(void)
{
    ESP_LOGI(TAG, "SYSTEM_EVENT_STA_START event received...");
    (void)CHECK_EXE(esp_wifi_connect());

}

/**--------------------------------------------------------------------------------------
 * @brief     Event function for wifi station connected event
 * @author    S. Wink
 * @date      20. Sep. 2019
*//*-----------------------------------------------------------------------------------*/
static void CreateIpLink_vd(void)
{
    ESP_LOGI(TAG, "SYSTEM_EVENT_STA_CONNECTED event received...");
    // enable ipv6
    (void)CHECK_EXE(tcpip_adapter_create_ip6_linklocal(TCPIP_ADAPTER_IF_STA));
}

/**--------------------------------------------------------------------------------------
 * @brief     Event function for wifi station successful got ip 6
 * @author    S. Wink
 * @date      25. Jul. 2019
 * @param     event_stp     the received event
*//*-----------------------------------------------------------------------------------*/
static void PrintReceivedIp_vd(system_event_t *event_stp)
{
    char *ip6 = ip6addr_ntoa(&event_stp->event_info.got_ip6.ip6_info.ip);

    ESP_LOGI(TAG, "SYSTEM_EVENT_STA_GOT_IP6 event received...");
    ESP_LOGI(TAG, "IPv6: %s", ip6);
}


/****************************************************************************************
* FILENAME :        wifiCtrl.c
*
* DESCRIPTION :
*       This module handles the WIFI connection.
*
* AUTHOR :    Stephan Wink        CREATED ON :    24.01.2019
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
#include "wifiCtrl.h"

/***************************************************************************************/
/* Local constant defines */
#define DEFAULT_WIFI_SSID_STA       CONFIG_WIFI_SSID
#define DEFAULT_WIFI_PASS_STA       CONFIG_WIFI_PASSWORD
#define MAXIMUM_NUM_OF_RETRIES      CONFIG_ESP_MAXIMUM_RETRY
#define MAXIMUM_CONN_RETRIES        2
#define DEFAULT_WIFI_SSID_AP        "myssid3_"
#define DEFAULT_WIFI_PASS_AP        "testtest"
#define MAXIMUM_AP_CONNECTIONS      2
#define SIZE_OF_SSID_VECTOR         32
#define SIZE_OF_PASSWORD            64

/***************************************************************************************/
/* Local function like makros */

/***************************************************************************************/
/* Local type definitions (enum, struct, union) */

typedef struct wifiParam_tag
{
    uint8_t ssid[SIZE_OF_SSID_VECTOR];           /**< SSID of ESP32 soft-AP */
    uint8_t password[SIZE_OF_PASSWORD];       /**< Password of ESP32 soft-AP */
}wifiPrarm_t;

/***************************************************************************************/
/* Local functions prototypes: */
static int32_t CmdHandlerStartStationMode_s32(int32_t argc_s32, char** argv);
static int32_t CmdHandlerStartAPMode_s32(int32_t argc_s32, char** argv);
static int32_t CmdHandlerChangeParameter_s32(int32_t argc_s32, char** argv);
static esp_err_t EventHandler_st(void *ctx_vp, system_event_t *event_stp);
static esp_err_t HandleStationStartRequestEvent_st(system_event_t *event);
static esp_err_t HandleStationConnectedEvent_st(system_event_t *event);
static esp_err_t HandleStationGotIpEvent_st(system_event_t *event);
static esp_err_t HandleStationGotIp6Event_st(system_event_t *event);
static esp_err_t HandleWifiStationDisconnect_st(system_event_t *event);
static esp_err_t HandleWifiClientConnected_st(system_event_t *event);
static esp_err_t HandleWifiClientDisConnected_st(system_event_t *event);
static esp_err_t HandleUnexpectedWifiEvent_st(system_event_t *event);

/***************************************************************************************/
/* Local variables: */
static const char *TAG = "myWifi";

static EventGroupHandle_t wifiEventGroup_sts;
static uint8_t retryConnectCounter_u8st = 0U;

static void (*eventWifiStarted_ptrs)(void);
static void (*eventWifiStartedSta_ptrs)(void);
static void (*eventWifiDisconnected_ptrs)(void);

static wifi_config_t stationWifiSettings_sts =
{
    .sta =
    {
        .ssid = DEFAULT_WIFI_SSID_STA,
        .password = DEFAULT_WIFI_PASS_STA
    },
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

/** Arguments used by 'parameter' function */
static struct
{
    struct arg_int *timeout_stp;
    struct arg_str *ssid_stp;
    struct arg_str *password_stp;
    struct arg_end *end_stp;
}cmdArgsStation_sts;

static const char *WIFI_PARA_IDENT = "wifi";
static paramif_objHdl_t wifiParaHdl_xps;
static wifiPrarm_t wifiParam_sts;
static const wifiPrarm_t defaultParam_stsc =
{
    .ssid = DEFAULT_WIFI_SSID_STA,
    .password = DEFAULT_WIFI_PASS_STA
};
/***************************************************************************************/
/* Global functions (unlimited visibility) */

/**--------------------------------------------------------------------------------------
 * @brief     General initialization of wifi module. This function has to be
 *              executed before the other ones.
*//*-----------------------------------------------------------------------------------*/
void wifiCtrl_InitializeWifi_vd(wifiIf_eventCallB_t *param_stp)
{
    paramif_allocParam_t wifiAllocParam_st;
    wifi_init_config_t cfg_st = WIFI_INIT_CONFIG_DEFAULT();

    esp_log_level_set("phy_init", ESP_LOG_INFO);

    eventWifiStarted_ptrs = param_stp->eventCallBackOnAPStarted_ptrs;
    eventWifiStartedSta_ptrs = param_stp->eventCallBackOnStationStarted_ptrs;
    eventWifiDisconnected_ptrs = param_stp->eventCallBackWifiDisconn_ptrs;

    tcpip_adapter_init();
    wifiEventGroup_sts = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_init(EventHandler_st, NULL));
    ESP_ERROR_CHECK(esp_wifi_init(&cfg_st));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

    ESP_ERROR_CHECK(paramif_InitializeAllocParameter_td(&wifiAllocParam_st));
    wifiAllocParam_st.length_u16 = sizeof(defaultParam_stsc);
    wifiAllocParam_st.defaults_u8p = (uint8_t *)&defaultParam_stsc;
    wifiAllocParam_st.nvsIdent_cp = WIFI_PARA_IDENT;
    wifiParaHdl_xps = paramif_Allocate_stp(&wifiAllocParam_st);
    paramif_PrintHandle_vd(wifiParaHdl_xps);
    ESP_ERROR_CHECK(paramif_Read_td(wifiParaHdl_xps, (uint8_t *) &wifiParam_sts));
    ESP_LOGI(TAG, "loaded parameter from nvs, ssid: %s, pwd: %s",
            wifiParam_sts.ssid,
            wifiParam_sts.password);
}

/**--------------------------------------------------------------------------------------
 * @brief     Function to initialize WIFI to access point mode
*//*-----------------------------------------------------------------------------------*/
void wifiCtrl_InitializeWifiSoftAp_vd(void)
{
    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_ERROR_CHECK(esp_wifi_deinit());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &apWifiSettings_sts));
    ESP_ERROR_CHECK(esp_wifi_start());
    retryConnectCounter_u8st = 0U;

    ESP_LOGI(TAG, "wifi_init_softap finished.SSID:%s password:%s",
             DEFAULT_WIFI_SSID_AP, DEFAULT_WIFI_PASS_AP);
}

/**--------------------------------------------------------------------------------------
 * @brief     Function to initialize WIFI to station mode
*//*-----------------------------------------------------------------------------------*/
void wifiCtrl_InitializeWifiSta_vd(void)
{
    //paramif_PrintHandle_vd(wifiParaHdl_xps);
    ESP_ERROR_CHECK(paramif_Read_td(wifiParaHdl_xps, (uint8_t *) &wifiParam_sts));
    ESP_LOGI(TAG, "loaded parameter from nvs, ssid: %s, pwd: %s",
                wifiParam_sts.ssid,
                wifiParam_sts.password);
    memset(stationWifiSettings_sts.sta.ssid, 0,
            sizeof(stationWifiSettings_sts.sta.ssid));
    memcpy(&stationWifiSettings_sts.sta.ssid[0],
            &wifiParam_sts.ssid[0],
            sizeof(stationWifiSettings_sts.sta.ssid));
    memset(stationWifiSettings_sts.sta.password, 0,
            sizeof(stationWifiSettings_sts.sta.password));
    memcpy(&stationWifiSettings_sts.sta.password[0],
            &wifiParam_sts.password[0],
            sizeof(stationWifiSettings_sts.sta.password));

    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &stationWifiSettings_sts));
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");
    ESP_LOGI(TAG, "connect to ap SSID:%s password:%s",
             DEFAULT_WIFI_SSID_STA, DEFAULT_WIFI_PASS_STA);
}

/**--------------------------------------------------------------------------------------
 * @brief     Function to initialize WIFI to station mode
 * @author    S. Wink
 * @date      24. Jan. 2019
*//*-----------------------------------------------------------------------------------*/
void wifiCtrl_RegisterWifiCommands(void)
{
    cmdArgsStation_sts.timeout_stp = arg_int0("t", "timeout", "<t>", "Connection timeout, ms");
    cmdArgsStation_sts.timeout_stp->ival[0] = 5000; // set default value
    cmdArgsStation_sts.ssid_stp = arg_str1(NULL, NULL, "<ssid>", "SSID of AP");
    cmdArgsStation_sts.password_stp = arg_str0(NULL, NULL, "<pass>", "PSK of AP");
    cmdArgsStation_sts.end_stp = arg_end(2);

    const myConsole_cmd_t paramCmd = {
        .command = "wifi",
        .help = "Wifi station parameter setup",
        .hint = NULL,
        .func = &CmdHandlerChangeParameter_s32,
        .argtable = &cmdArgsStation_sts
    };

    ESP_ERROR_CHECK(myConsole_CmdRegister_td(&paramCmd));

    const myConsole_cmd_t accessPointCmd = {
        .command = "ap",
        .help = "Start Wifi as access point",
        .func = &CmdHandlerStartAPMode_s32,
    };

    ESP_ERROR_CHECK(myConsole_CmdRegister_td(&accessPointCmd));

    const myConsole_cmd_t stationCmd = {
            .command = "stat",
            .help = "Start Wifi and join the Access Point",
            .func = &CmdHandlerStartStationMode_s32,
        };

        ESP_ERROR_CHECK(myConsole_CmdRegister_td(&stationCmd));
}

/***************************************************************************************/
/* Local functions: */

/**--------------------------------------------------------------------------------------
 * @brief     Handler for console command start station mode
 * @author    S. Wink
 * @date      24. Jan. 2019
 * @param     argc_s32  count of argument list
 * @param     argv      pointer to argument list
 * @return    not equal to zero if error detected
*//*-----------------------------------------------------------------------------------*/
static int32_t CmdHandlerStartStationMode_s32(int32_t argc_s32, char** argv)
{
    wifiCtrl_InitializeWifiSta_vd();
    return 0;
}

/**--------------------------------------------------------------------------------------
 * @brief     Handler for console command start access point
 * @author    S. Wink
 * @date      24. Jan. 2019
 * @param     argc_s32  count of argument list
 * @param     argv      pointer to argument list
 * @return    not equal to zero if error detected
*//*-----------------------------------------------------------------------------------*/
static int32_t CmdHandlerStartAPMode_s32(int32_t argc_s32, char** argv)
{
    wifiCtrl_InitializeWifiSoftAp_vd();
    return 0;
}

/**--------------------------------------------------------------------------------------
 * @brief     Handler for console command parameter set
 * @author    S. Wink
 * @date      24. Jan. 2019
 * @param     argc_s32  count of argument list
 * @param     argv      pointer to argument list
 * @return    not equal to zero if error detected
*//*-----------------------------------------------------------------------------------*/
static int32_t CmdHandlerChangeParameter_s32(int32_t argc_s32, char** argv)
{
    int nerrors_i = arg_parse(argc_s32, argv, (void**) &cmdArgsStation_sts);
    if (nerrors_i != 0) {
        arg_print_errors(stderr, cmdArgsStation_sts.end_stp, argv[0]);
        return 1;
    }

    memset(wifiParam_sts.ssid, 0, sizeof(wifiParam_sts.ssid));
    memcpy(wifiParam_sts.ssid, cmdArgsStation_sts.ssid_stp->sval,
            sizeof(stationWifiSettings_sts.sta.ssid));

    memset(wifiParam_sts.password, 0, sizeof(wifiParam_sts.password));
    memcpy(wifiParam_sts.password, cmdArgsStation_sts.password_stp->sval,
            sizeof(wifiParam_sts.password));

    ESP_LOGI(TAG, "SSID: %s, PWD: %s", wifiParam_sts.ssid, wifiParam_sts.password);
    return(0);
}

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

    // execute the assigned wifi started callback function
    if(NULL != eventWifiStarted_ptrs)
    {
        (*eventWifiStartedSta_ptrs)();
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
        if(NULL != eventWifiDisconnected_ptrs)
        {
            (*eventWifiDisconnected_ptrs)();
        }
        /* This is a workaround as ESP32 WiFi libs don't currently
        auto-reassociate.*/
        esp_wifi_connect();
        ESP_LOGI(TAG,"retry %d to connect to the AP",
                            retryConnectCounter_u8st);
        retryConnectCounter_u8st++;
    }
    else
    {
        retryConnectCounter_u8st = 0U;
        ESP_LOGI(TAG,"connection to access point failed, start own access point");
        wifiCtrl_InitializeWifiSoftAp_vd();
    }

    return(ESP_OK);
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
    if(NULL != eventWifiStarted_ptrs)
    {
        (*eventWifiStarted_ptrs)();
        ESP_LOGI(TAG, "executed wifi ap connect callback function...");
    }

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
    if(NULL != eventWifiDisconnected_ptrs)
    {
        (*eventWifiDisconnected_ptrs)();
        ESP_LOGI(TAG, "executed wifi ap disconnect callback function...");
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

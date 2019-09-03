/****************************************************************************************
* FILENAME :        wifiDrv.c
*
* DESCRIPTION :
*       This module handles the WIFI connection.
*
* AUTHOR :    Stephan Wink        CREATED ON :    01.09.2019
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
#include "wifiStation.h"
#include "wifiAp.h"
#include "wifiDrv.h"

/***************************************************************************************/
/* Local constant defines */

#ifndef BIT0
    #define BIT0    0x00000000
    #define BIT1    0x00000001
    #define BIT2    0x00000002
    #define BIT3    0x00000004
    #define BIT4    0x00000008
#endif

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

typedef enum objectState_tag
{
     STATE_NOT_INITIALIZED,
     STATE_INITIALIZED,
     STATE_CONNECTED_TO_STATION,
     STATE_DISCONNECTED_FROM_STATION,
     STATE_CONNECTED_TO_CLIENT,
     STATE_ACCESS_POINT_PENDING
}objectState_t;

typedef struct wifiParam_tag
{
    uint8_t ssid[SIZE_OF_SSID_VECTOR];           /**< SSID of ESP32 soft-AP */
    uint8_t password[SIZE_OF_PASSWORD];       /**< Password of ESP32 soft-AP */
}wifiPrarm_t;

typedef struct objectData_tag
{
     objectState_t state_en;
     wifiIf_setStation_t setStation_st;
     wifiIf_setAp_t setAccessPoint_st;
     wifiIf_eventCallB2_t callerCb_st;
}objectData_t;

/***************************************************************************************/
/* Local functions prototypes: */
static void EventConnectedToStation(void);
static void EventDisconnectedFromStation(void);
static void EventConnectedClient(void);
static void EventDisconnectedClient(void);
static void Task_vd(void *pvParameters);
static void HandleStartOfWifi(void);
static void HandleConnectedToStation(void);
static void HandleDisconnectedFromStation(void);
static void HandleClientConnected(void);
static void HandleClientDisconnected(void);
static int32_t CmdHandlerStartStationMode_s32(int32_t argc_s32, char** argv);
static int32_t CmdHandlerStartAPMode_s32(int32_t argc_s32, char** argv);
static int32_t CmdHandlerChangeParameter_s32(int32_t argc_s32, char** argv);
static void ConnectToStation(void);
static void OpenAccessPoint(void);

/***************************************************************************************/
/* Local variables: */
static const char *TAG = "wifiDrv";

static const int START              = BIT0;
static const int STATION_CONN       = BIT1;
static const int STATION_DISCON     = BIT2;
static const int CLIENT_CONN        = BIT3;
static const int CLIENT_DISCON      = BIT4;

static objectData_t singleton_sst =
{
    .state_en = STATE_NOT_INITIALIZED,
    .setStation_st =
    {
        .wifiSettings_st =
        {
            .sta =
            {
                .ssid = DEFAULT_WIFI_SSID_STA,
                .password = DEFAULT_WIFI_PASS_STA
            },
        },
        .wifiCallBacks_st =
        {
            .callBackStationConnected_fp = EventConnectedToStation,
            .callBackStationDisconn_fp = EventDisconnectedFromStation
        }
    },
    .setAccessPoint_st =
    {
        .wifiSettings_st =
        {
            {
                .ssid = DEFAULT_WIFI_SSID_AP,
                .ssid_len = strlen(DEFAULT_WIFI_SSID_AP),
                .password = DEFAULT_WIFI_PASS_AP,
                .max_connection = MAXIMUM_AP_CONNECTIONS,
                .authmode = WIFI_AUTH_WPA_WPA2_PSK
            },
        },
        .wifiCallBacks_st =
        {
            .callBackClientConnected_fp = EventConnectedClient,
            .callBackClientDisconn_fp = EventDisconnectedClient

        }
    }
};

static EventGroupHandle_t wifiEventGroup_sts;

static uint8_t retryConnectCounter_u8st = 0U;

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

/**---------------------------------------------------------------------------------------
 * @brief     Initializes the initialization structure of the mqtt module
*//*-----------------------------------------------------------------------------------*/
esp_err_t wifiDrv_InitializeParameter(wifiIf_eventCallB2_t *param_stp)
{
    memset(param_stp, 0U, sizeof(*param_stp));

    return(ESP_OK);
}

/**--------------------------------------------------------------------------------------
 * @brief     General initialization of wifi module. This function has to be
 *              executed before the other ones.
*//*-----------------------------------------------------------------------------------*/
esp_err_t wifiDrv_Initialize_vd(wifiIf_eventCallB2_t *param_stp)
{
    esp_err_t result_st = ESP_OK;
    paramif_allocParam_t wifiAllocParam_st;

    esp_log_level_set("phy_init", ESP_LOG_INFO);

    if(NULL != param_stp)
    {
        memcpy(&singleton_sst.callerCb_st, param_stp, sizeof(singleton_sst.callerCb_st));
    }

    /* if needed use the event handler as callback as well
    tcpip_adapter_init();
    wifiEventGroup_sts = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_init(EventHandler_st, NULL));
    ESP_ERROR_CHECK(esp_wifi_init(&cfg_st));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM)); */

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

    wifiEventGroup_sts = xEventGroupCreate();
    if(NULL == wifiEventGroup_sts)
    {
        result_st = ESP_FAIL;
        ESP_LOGE(TAG, "mqtt client init event alloc failed...");
    }

    /* start the mqtt task */
    xTaskCreate(Task_vd, "wifiTask", 4096, NULL, 4, NULL);

    ESP_LOGD(TAG, "initialize done...");
    singleton_sst.state_en = STATE_INITIALIZED;

    return(result_st);
}

/**---------------------------------------------------------------------------------------
 * @brief     Starts the wifi demon if it was initialized
*//*-----------------------------------------------------------------------------------*/
void wifidrv_StartWifiDemon(void)
{
    if(STATE_INITIALIZED == singleton_sst.state_en)
    {
        xEventGroupSetBits(wifiEventGroup_sts, START);
    }
}

/**--------------------------------------------------------------------------------------
 * @brief     Function to register all console wifi commands
 * @author    S. Wink
 * @date      24. Jan. 2019
*//*-----------------------------------------------------------------------------------*/
void wifiDrv_RegisterWifiCommands_vd(void)
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
/**---------------------------------------------------------------------------------------
 * @brief     event callback function for station connection
 * @author    S. Wink
 * @date      01. Sep. 2019
*//*-----------------------------------------------------------------------------------*/
static void EventConnectedToStation(void)
{
    xEventGroupSetBits(wifiEventGroup_sts, STATION_CONN);
}

/**---------------------------------------------------------------------------------------
 * @brief     event callback function for station disconnection
 * @author    S. Wink
 * @date      01. Sep. 2019
*//*-----------------------------------------------------------------------------------*/
static void EventDisconnectedFromStation(void)
{
    xEventGroupSetBits(wifiEventGroup_sts, STATION_DISCON);
}

/**---------------------------------------------------------------------------------------
 * @brief     event callback function for client connection
 * @author    S. Wink
 * @date      01. Sep. 2019
*//*-----------------------------------------------------------------------------------*/
static void EventConnectedClient(void)
{
    xEventGroupSetBits(wifiEventGroup_sts, CLIENT_CONN);
}

/**---------------------------------------------------------------------------------------
 * @brief     event callback function for client connection
 * @author    S. Wink
 * @date      01. Sep. 2019
*//*-----------------------------------------------------------------------------------*/
static void EventDisconnectedClient(void)
{
    xEventGroupSetBits(wifiEventGroup_sts, CLIENT_DISCON);
}

/**---------------------------------------------------------------------------------------
 * @brief     task routine for the wifi handling
 * @author    S. Wink
 * @date      01. Sep. 2019
 * @param     pvParameters      interface variable from freertos
*//*-----------------------------------------------------------------------------------*/
static void Task_vd(void *pvParameters)
{
    EventBits_t uxBits_st;
    uint32_t bits_u32 = START | STATION_CONN | CLIENT_CONN | CLIENT_DISCON;

    ESP_LOGD(TAG, "wifiTask started...");
    while(1)
    {
        uxBits_st = xEventGroupWaitBits(wifiEventGroup_sts, bits_u32,
                                         true, false, portMAX_DELAY); // @suppress("Symbol is not resolved")

        if(0 != (uxBits_st & START))
        {
            HandleStartOfWifi();
        }
        if(0 != (uxBits_st & STATION_CONN))
        {
            HandleConnectedToStation();
        }
        if(0 != (uxBits_st & STATION_DISCON))
        {
            HandleDisconnectedFromStation();
        }
        if(0 != (uxBits_st & CLIENT_CONN))
        {
            HandleClientConnected();

        }
        if(0 != (uxBits_st & CLIENT_DISCON))
        {
            HandleClientDisconnected();
        }
    }
}

/**---------------------------------------------------------------------------------------
 * @brief     event handle for wifi start point
 * @author    S. Wink
 * @date      01. Sep. 2019
*//*-----------------------------------------------------------------------------------*/
static void HandleStartOfWifi(void)
{
    if(STATE_NOT_INITIALIZED != singleton_sst.state_en)
    {
        ConnectToStation();
    }
}

/**---------------------------------------------------------------------------------------
 * @brief     event handle for station connection
 * @author    S. Wink
 * @date      01. Sep. 2019
*//*-----------------------------------------------------------------------------------*/
static void HandleConnectedToStation(void)
{
    singleton_sst.state_en = STATE_CONNECTED_TO_STATION;
    retryConnectCounter_u8st = 0U;
    if(NULL != singleton_sst.callerCb_st.eventCallBOnStationConn_fp)
    {
        (*singleton_sst.callerCb_st.eventCallBOnStationConn_fp)();
    }
}

/**---------------------------------------------------------------------------------------
 * @brief     event handle for station disconnection
 * @author    S. Wink
 * @date      01. Sep. 2019
*//*-----------------------------------------------------------------------------------*/
static void HandleDisconnectedFromStation(void)
{
    singleton_sst.state_en = STATE_CONNECTED_TO_STATION;
    retryConnectCounter_u8st++;
    if(MAXIMUM_CONN_RETRIES == retryConnectCounter_u8st)
    {
        OpenAccessPoint();
        if(NULL != singleton_sst.callerCb_st.eventCallBackWifiDisconn_fp)
        {
            (*singleton_sst.callerCb_st.eventCallBackWifiDisconn_fp)();
        }
    }
    else
    {
        ConnectToStation();
    }
}

/**---------------------------------------------------------------------------------------
 * @brief     event handle for client connection to access point
 * @author    S. Wink
 * @date      01. Sep. 2019
*//*-----------------------------------------------------------------------------------*/
static void HandleClientConnected(void)
{
    singleton_sst.state_en = STATE_CONNECTED_TO_CLIENT;
    if(NULL != singleton_sst.callerCb_st.eventCallBOnClientConn_fp)
    {
        (*singleton_sst.callerCb_st.eventCallBOnClientConn_fp)();
    }
}

/**---------------------------------------------------------------------------------------
 * @brief     event handle for client disconnection from access point
 * @author    S. Wink
 * @date      01. Sep. 2019
*//*-----------------------------------------------------------------------------------*/
static void HandleClientDisconnected(void)
{
    singleton_sst.state_en = STATE_ACCESS_POINT_PENDING;
    if(NULL != singleton_sst.callerCb_st.eventCallBackWifiDisconn_fp)
    {
        (*singleton_sst.callerCb_st.eventCallBackWifiDisconn_fp)();
    }
}

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
    ConnectToStation();
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
    OpenAccessPoint();
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
    int32_t exeResult_s32 = 0U;
    wifi_sta_config_t * cfg_stp = &singleton_sst.setStation_st.wifiSettings_st.sta;
    int32_t nErrors_s32 = 0;

    nErrors_s32 = arg_parse(argc_s32, argv, (void**) &cmdArgsStation_sts);

    if (0 != nErrors_s32)
    {
        arg_print_errors(stderr, cmdArgsStation_sts.end_stp, argv[0]);
        exeResult_s32 = 1;
    }
    else
    {
        memset(cfg_stp->ssid, 0, sizeof(cfg_stp->ssid));
        memcpy(cfg_stp->ssid, cmdArgsStation_sts.ssid_stp->sval, sizeof(cfg_stp->ssid));

        memset(cfg_stp->password, 0, sizeof(cfg_stp->password));
        memcpy(cfg_stp->password, cmdArgsStation_sts.password_stp->sval,
                sizeof(cfg_stp->password));

        ESP_LOGI(TAG, "SSID: %s, PWD: %s", cfg_stp->ssid, cfg_stp->password);
        exeResult_s32 = 0;
    }

    return(exeResult_s32);
}

/**--------------------------------------------------------------------------------------
 * @brief     Connect to a preconfigured wifi station
 * @author    S. Wink
 * @date      01. Sep. 2019
 * @return    -
*//*-----------------------------------------------------------------------------------*/
static void ConnectToStation(void)
{
    wifiStation_Initialize_st(&singleton_sst.setStation_st);
}

/**--------------------------------------------------------------------------------------
 * @brief     Opens an access points for clients
 * @author    S. Wink
 * @date      01. Sep. 2019
 * @return    -
*//*-----------------------------------------------------------------------------------*/
static void OpenAccessPoint(void)
{
    wifiAp_Initialize_st(&singleton_sst.setAccessPoint_st);
}



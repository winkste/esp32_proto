/*****************************************************************************************
* FILENAME :        myWifi.c
*
* DESCRIPTION :
*       This module handles the WIFI connection.
*
* AUTHOR :    Stephan Wink        CREATED ON :    24.01.2019
*
* PUBLIC FUNCTIONS :
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
*****************************************************************************************/

/****************************************************************************************/
/* Include Interfaces */
#include "myWifi.h"

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

/****************************************************************************************/
/* Local constant defines */
#define EXAMPLE_ESP_WIFI_SSID      CONFIG_WIFI_SSID//"test"
#define EXAMPLE_ESP_WIFI_PASS      CONFIG_WIFI_PASSWORD
#define EXAMPLE_ESP_MAXIMUM_RETRY  CONFIG_ESP_MAXIMUM_RETRY
#define CONFIG_MAX_CONN_RETRIES       2
#define EXAMPLE_ESP_AP_WIFI_SSID      "myssid3_"
#define EXAMPLE_ESP_AP_WIFI_PASS      "testtest"
#define EXAMPLE_MAX_AP_STA_CONN       2

#ifndef BIT0
    #define BIT0    0x00000000
    #define BIT1    0x00000001
#define BIT1        0x00000002
#endif

/****************************************************************************************/
/* Local function like makros */

/****************************************************************************************/
/* Local type definitions (enum, struct, union) */

typedef struct wifiParam_tag
{
    uint8_t ssid[32];           /**< SSID of ESP32 soft-AP */
    uint8_t password[64];       /**< Password of ESP32 soft-AP */
}wifiPrarm_t;

/****************************************************************************************/
/* Local functions prototypes: */
static int stationCommandHandler_i(int argc, char** argv);
static int apCommandHandler_i(int argc, char** argv);
static int paramCommandHandler_i(int argc, char** argv);
static esp_err_t EventHandler_st(void *ctx, system_event_t *event);

/****************************************************************************************/
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
        .ssid = EXAMPLE_ESP_WIFI_SSID,
        .password = EXAMPLE_ESP_WIFI_PASS
    },
};

static wifi_config_t apWifiSettings_sts =
{
    .ap =
    {
        .ssid = EXAMPLE_ESP_AP_WIFI_SSID,
        .ssid_len = strlen(EXAMPLE_ESP_AP_WIFI_SSID),
        .password = EXAMPLE_ESP_AP_WIFI_PASS,
        .max_connection = EXAMPLE_MAX_AP_STA_CONN,
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
    .ssid = EXAMPLE_ESP_WIFI_SSID,
    .password = EXAMPLE_ESP_WIFI_PASS
};
/****************************************************************************************/
/* Global functions (unlimited visibility) */

/**---------------------------------------------------------------------------------------
 * @brief     General initialization of wifi module. This function has to be
 *              executed before the other ones.
*//*-----------------------------------------------------------------------------------*/
void myWifi_InitializeWifi_vd(myWifi_parameter_t *param_stp)
{
    eventWifiStarted_ptrs = param_stp->eventWifiStarted_ptrs;
    eventWifiStartedSta_ptrs = param_stp->eventWifiStartedSta_ptrs;
    eventWifiDisconnected_ptrs = param_stp->eventWifiDisconnected_ptrs;
    paramif_allocParam_t wifiAllocParam_st;

    esp_log_level_set("phy_init", ESP_LOG_INFO);

    tcpip_adapter_init();
    wifiEventGroup_sts = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_init(EventHandler_st, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
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

/**---------------------------------------------------------------------------------------
 * @brief     Function to initialize WIFI to access point mode
*//*-----------------------------------------------------------------------------------*/
void myWifi_InitializeWifiSoftAp_vd(void)
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
             EXAMPLE_ESP_AP_WIFI_SSID, EXAMPLE_ESP_AP_WIFI_PASS);
}

/**---------------------------------------------------------------------------------------
 * @brief     Function to initialize WIFI to station mode
*//*-----------------------------------------------------------------------------------*/
void myWifi_InitializeWifiSta_vd(void)
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
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
}

/**---------------------------------------------------------------------------------------
 * @brief     Function to initialize WIFI to station mode
 * @author    S. Wink
 * @date      24. Jan. 2019
*//*-----------------------------------------------------------------------------------*/
void myWifi_RegisterWifiCommands(void)
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
        .func = &paramCommandHandler_i,
        .argtable = &cmdArgsStation_sts
    };

    ESP_ERROR_CHECK(myConsole_CmdRegister_td(&paramCmd));

    const myConsole_cmd_t accessPointCmd = {
        .command = "ap",
        .help = "Start Wifi as access point",
        .func = &apCommandHandler_i,
    };

    ESP_ERROR_CHECK(myConsole_CmdRegister_td(&accessPointCmd));

    const myConsole_cmd_t stationCmd = {
            .command = "stat",
            .help = "Start Wifi and join the Access Point",
            .func = &stationCommandHandler_i,
        };

        ESP_ERROR_CHECK(myConsole_CmdRegister_td(&stationCmd));
}

/****************************************************************************************/
/* Local functions: */

/**---------------------------------------------------------------------------------------
 * @brief     Handler for console command start station mode
 * @author    S. Wink
 * @date      24. Jan. 2019
 * @param     argc  count of argument list
 * @param     argv  pointer to argument list
 * @return    not equal to zero if error detected
*//*-----------------------------------------------------------------------------------*/
static int stationCommandHandler_i(int argc, char** argv)
{
    myWifi_InitializeWifiSta_vd();
    return 0;
}

/**---------------------------------------------------------------------------------------
 * @brief     Handler for console command start access point
 * @author    S. Wink
 * @date      24. Jan. 2019
 * @param     argc  count of argument list
 * @param     argv  pointer to argument list
 * @return    not equal to zero if error detected
*//*-----------------------------------------------------------------------------------*/
static int apCommandHandler_i(int argc, char** argv)
{
    myWifi_InitializeWifiSoftAp_vd();
    return 0;
}

/**---------------------------------------------------------------------------------------
 * @brief     Handler for console command parameter set
 * @author    S. Wink
 * @date      24. Jan. 2019
 * @param     argc  count of argument list
 * @param     argv  pointer to argument list
 * @return    not equal to zero if error detected
*//*-----------------------------------------------------------------------------------*/
static int paramCommandHandler_i(int argc, char** argv)
{
    int nerrors_i = arg_parse(argc, argv, (void**) &cmdArgsStation_sts);
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

/**---------------------------------------------------------------------------------------
 * @brief     Event handler for WIFI events
 * @author    S. Wink
 * @date      24. Jan. 2019
 * @param     ctx       ???
 * @param     event     the received event
 * @return    a esp_err_t based error code, currently only ESP_OK
*//*-----------------------------------------------------------------------------------*/
static esp_err_t EventHandler_st(void *ctx, system_event_t *event)
{
    ESP_LOGI(TAG, "event handler called");

    switch(event->event_id)
    {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        ESP_LOGI(TAG, "SYSTEM_EVENT_STA_START event received...");
        break;
    case SYSTEM_EVENT_STA_CONNECTED:
        ESP_LOGI(TAG, "SYSTEM_EVENT_STA_CONNECTED event received...");
        // enable ipv6
        tcpip_adapter_create_ip6_linklocal(TCPIP_ADAPTER_IF_STA);
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        ESP_LOGI(TAG, "SYSTEM_EVENT_STA_GOT_IP event received...");
        break;
    case SYSTEM_EVENT_AP_STA_GOT_IP6:
        ESP_LOGI(TAG, "SYSTEM_EVENT_STA_GOT_IP6 event received...");
        char *ip6 = ip6addr_ntoa(&event->event_info.got_ip6.ip6_info.ip); // @suppress("Field cannot be resolved")
        ESP_LOGI(TAG, "IPv6: %s", ip6);
        if(NULL != eventWifiStarted_ptrs)
        {
            (*eventWifiStartedSta_ptrs)();
        }
        ESP_LOGI(TAG, "callBack function for wifi start executed...");
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        ESP_LOGI(TAG, "SYSTEM_EVENT_STA_DISCONNECTED event received...");
        if(CONFIG_MAX_CONN_RETRIES > retryConnectCounter_u8st)
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
            ESP_LOGI(TAG,"connection to AP failed, start own AP");
            myWifi_InitializeWifiSoftAp_vd();
        }
        break;
    case SYSTEM_EVENT_AP_STACONNECTED:
        ESP_LOGI(TAG, "SYSTEM_EVENT_AP_STACONNECTED event received...");
        ESP_LOGI(TAG, "station2:"MACSTR" join, AID=%d",
                 MAC2STR(event->event_info.sta_connected.mac),
                 event->event_info.sta_connected.aid);
        if(NULL != eventWifiDisconnected_ptrs)
        {
            (*eventWifiStarted_ptrs)();
        }
        ESP_LOGI(TAG, "START_SOCKET_SERVER event fired...");
        break;
    case SYSTEM_EVENT_AP_STADISCONNECTED:
        ESP_LOGI(TAG, "SYSTEM_EVENT_AP_STADISCONNECTED event received...");
        ESP_LOGI(TAG, "station:"MACSTR"leave, AID=%d",
                 MAC2STR(event->event_info.sta_disconnected.mac),
                 event->event_info.sta_disconnected.aid);
        if(NULL != eventWifiDisconnected_ptrs)
        {
            (*eventWifiDisconnected_ptrs)();
        }
        break;
    default:
        break;
    }
    return ESP_OK;
}

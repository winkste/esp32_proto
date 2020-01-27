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

#include "wifiCtrl.h"

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
#include "wifiAp.h"
#include "wifiStation.h"

/***************************************************************************************/
/* Local constant defines */
#define DEFAULT_WIFI_SSID_STA       wifiIf_DEFAULT_WIFI_SSID_STA
#define DEFAULT_WIFI_PASS_STA       wifiIf_DEFAULT_WIFI_PASS_STA
#define MAXIMUM_CONN_RETRIES        3

#define MODULE_TAG                  "wifiCtrl"

#define CHECK_EXE(arg) utils_CheckAndLogExecution_bol(MODULE_TAG, arg, __LINE__)

/***************************************************************************************/
/* Local function like makros */

/***************************************************************************************/
/* Local type definitions (enum, struct, union) */

typedef struct wifiCtrl_serviceObj_tag
{
     wifiIf_service_t service_st;
     bool active_bol;
     wifiCtrl_serviceHdl_t next_xp;
     wifiCtrl_serviceHdl_t last_xp;
}wifiCtrl_serviceObj_t;

typedef enum objectState_tag
{
    STATE_NOT_INITIALIZED,
    STATE_INITIALIZED,
    STATE_CONNECTED,
    STATE_DISCONNECTED,
    STATE_WAITING_FOR_CLIENT,
    STATE_CLIENT_CONNECTED,
    STATE_WIFI_STOPPED
}objectState_t;

typedef struct objectData_tag
{
    objectState_t state_en;
    uint8_t connectRetries_u8;
    TimerHandle_t timer_xp;
    EventGroupHandle_t wifiEventGroup_xp;
    TaskHandle_t task_xp;
    wifiIf_service_t service_st;
    wifiIf_Converter_fcp Converter_fcp;
    wifiCtrl_serviceHdl_t service_xp;
}objectData_t;


typedef enum wifiMode_tag
{
    MODE_STATION,
    MODE_ACCESSPOINT,
    MODE_COMBINED
}wifiMode_t;

/***************************************************************************************/
/* Local functions prototypes: */

static void AddServiceToList_vd(wifiCtrl_serviceHdl_t hdl_xp);
//static void RemoveSubsFromList_vd(wifiCtrl_serviceHdl_t hdl_xp);
static esp_err_t Reconnect_st(void);
static void Task_vd(void *pvParameters);
static int32_t CmdHandlerChangeParameter_s32(int32_t argc_s32, char** argv);
static int32_t CmdHandlerChangeMode_s32(int32_t argc_s32, char** argv);
static esp_err_t EventHandler_td(void *ctx_vp, system_event_t *event_stp);
static void HandleConnectionTimeout_vd(TimerHandle_t xTimer);
static esp_err_t SetAndCheckState_td(objectState_t state_en);
static esp_err_t StartTimeout_td(void);
static esp_err_t CreateTimer_td(void);
static esp_err_t CreateEventGroup_td(void);
static esp_err_t CreateTask_td(void);

/***************************************************************************************/
/* Local variables: */
static const char *TAG = MODULE_TAG;

static objectData_t this_sst =
{
    .state_en = STATE_NOT_INITIALIZED,
    .connectRetries_u8 = 0U,
    .timer_xp = NULL,
    .wifiEventGroup_xp = NULL,
    .task_xp = NULL,
    .Converter_fcp = wifiAp_EventConverter_u32,
    .service_xp = NULL
};

/** Arguments used by 'parameter' function */
static struct
{
    struct arg_int *timeout_stp;
    struct arg_str *ssid_stp;
    struct arg_str *password_stp;
    struct arg_end *end_stp;
}cmdArgsStation_sts;

static struct
{
    struct arg_int *mode_stp;
    struct arg_end *end_stp;
}cmdArgsMode_sts;

static const char *STATION_PARA_IDENT = "wifi";
static paramif_objHdl_t stationParaHdl_xps;
static wifiIf_stationParam_t stationParam_sts;
static const wifiIf_stationParam_t stationDefaultParam_stsc =
{
    .ssid = wifiIf_DEFAULT_WIFI_SSID_STA,
    .password = wifiIf_DEFAULT_WIFI_PASS_STA
};

static const char *MODE_PARA_IDENT = "wifiMode";
static paramif_objHdl_t modeParaHdl_xps;
static wifiMode_t mode_ens = MODE_ACCESSPOINT;
static const wifiMode_t DEFAULT_MODE = MODE_ACCESSPOINT;

/***************************************************************************************/
/* Global functions (unlimited visibility) */

/**--------------------------------------------------------------------------------------
 * @brief     General initialization of wifi module. This function has to be
 *              executed before the other ones.
*//*-----------------------------------------------------------------------------------*/
esp_err_t wifiCtrl_Initialize_st(wifiIf_service_t *service_stp)
{
    esp_err_t result_st = ESP_FAIL;
    bool exeResult_bol = true;
    paramif_allocParam_t stationAllocParam_st;
    paramif_allocParam_t modeAllocParam_st;

    esp_log_level_set("phy_init", ESP_LOG_INFO);
    esp_log_level_set("wifi", ESP_LOG_WARN);
    esp_log_level_set("tcpip_adapter", ESP_LOG_INFO);
    //esp_log_level_set("wifiStation", ESP_LOG_WARN);
    //esp_log_level_set("wifiAp", ESP_LOG_WARN);
    //esp_log_level_set("wifiCtrl", ESP_LOG_WARN);

    memcpy(&this_sst.service_st, service_stp, sizeof(this_sst.service_st));
    this_sst.connectRetries_u8 = 0U;

    exeResult_bol &= CHECK_EXE(paramif_InitializeAllocParameter_td(&modeAllocParam_st));
    modeAllocParam_st.length_u16 = sizeof(mode_ens);
    modeAllocParam_st.defaults_u8p = (uint8_t *)&DEFAULT_MODE;
    modeAllocParam_st.nvsIdent_cp = MODE_PARA_IDENT;
    modeParaHdl_xps = paramif_Allocate_stp(&modeAllocParam_st);
    paramif_PrintHandle_vd(modeParaHdl_xps);
    exeResult_bol &= CHECK_EXE(paramif_Read_td(modeParaHdl_xps, (uint8_t *) &mode_ens));
    ESP_LOGI(TAG, "loaded wifi mode: %d", mode_ens);

    exeResult_bol &= CHECK_EXE(paramif_InitializeAllocParameter_td(
                                &stationAllocParam_st));
    stationAllocParam_st.length_u16 = sizeof(stationDefaultParam_stsc);
    stationAllocParam_st.defaults_u8p = (uint8_t *)&stationDefaultParam_stsc;
    stationAllocParam_st.nvsIdent_cp = STATION_PARA_IDENT;
    stationParaHdl_xps = paramif_Allocate_stp(&stationAllocParam_st);
    paramif_PrintHandle_vd(stationParaHdl_xps);
    exeResult_bol &= CHECK_EXE(paramif_Read_td(stationParaHdl_xps,
                                (uint8_t *) &stationParam_sts));
    ESP_LOGI(TAG, "loaded parameter from nvs, ssid: %s, pwd: %s",
            stationParam_sts.ssid,
            stationParam_sts.password);

    /*this_sst.timer_xp = xTimerCreate("WIFI_Timeout", pdMS_TO_TICKS(5000), false,
                                                (void *) 0, HandleConnectionTimeout_vd);
    if(NULL == this_sst.timer_xp)
    {
        exeResult_bol = false;
        ESP_LOGE(TAG, "wifi initialization failed to alloc task");
    }
    
    this_sst.wifiEventGroup_xp = xEventGroupCreate();
    if(NULL == this_sst.wifiEventGroup_xp)
    {
        exeResult_bol = false;
        ESP_LOGE(TAG, "wifi initialization failed to alloc event");
    }
    
    (void)xTaskCreate(Task_vd, "wifiTask", 4096, NULL, 4, &this_sst.task_xp);

    if(NULL == this_sst.task_xp)
    {
        exeResult_bol = false;
        ESP_LOGE(TAG, "wifi initialization failed to alloc task");    
    }*/

    exeResult_bol &= CHECK_EXE(CreateTimer_td());
    exeResult_bol &= CHECK_EXE(CreateEventGroup_td());
    exeResult_bol &= CHECK_EXE(CreateTask_td());

    exeResult_bol &= CHECK_EXE(esp_event_loop_init(EventHandler_td, NULL));

    if(true == exeResult_bol)
    {
        result_st = ESP_OK;
        SetAndCheckState_td(STATE_INITIALIZED);
    }

    return(result_st);
}

/**---------------------------------------------------------------------------------------
 * @brief     Function to start the wifi controlling
*//*-----------------------------------------------------------------------------------*/
esp_err_t wifiCtrl_Start_st(void)
{
    esp_err_t result_st = ESP_FAIL;
    bool exeResult_bol = true;

    if(STATE_NOT_INITIALIZED != this_sst.state_en)
    {
        this_sst.connectRetries_u8 = 0U;
        exeResult_bol &= CHECK_EXE(Reconnect_st());
    }
    else
    {
        exeResult_bol = false;
    }

    if(true == exeResult_bol)
    {
        result_st = ESP_OK;
    }
    return(result_st);
}

/**---------------------------------------------------------------------------------------
 * @brief     Function to stop the wifi 
*//*-----------------------------------------------------------------------------------*/
extern esp_err_t wifiCtrl_Stop_st(void)
{
    esp_err_t result_st = ESP_FAIL;
    bool exeResult_bol = true;

    if(MODE_STATION == mode_ens)
    {
        exeResult_bol = wifiStation_Stop_st();
    }
    else if(MODE_ACCESSPOINT)
    {
        exeResult_bol = wifiAp_Stop_st();
    }
    else
    {
        exeResult_bol = wifiAp_Stop_st();
    }


    if(true == exeResult_bol)
    {
        result_st = ESP_OK;
    }

    return(result_st);
}

/**--------------------------------------------------------------------------------------
 * @brief     Function to initialize WIFI to station mode
*//*-----------------------------------------------------------------------------------*/
void wifiCtrl_RegisterWifiCommands(void)
{
    cmdArgsStation_sts.timeout_stp = arg_int0("t", "timeout", "<t>",
                                                "Connection timeout, ms");
    cmdArgsStation_sts.timeout_stp->ival[0] = 5000; // set default value
    cmdArgsStation_sts.ssid_stp = arg_str1(NULL, NULL, "<ssid>", "SSID of AP");
    cmdArgsStation_sts.password_stp = arg_str0(NULL, NULL, "<pass>", "PSK of AP");
    cmdArgsStation_sts.end_stp = arg_end(2);

    const myConsole_cmd_t paramCmd = {
        .command = "stpa",
        .help = "Wifi station parameter setup",
        .hint = NULL,
        .func = &CmdHandlerChangeParameter_s32,
        .argtable = &cmdArgsStation_sts
    };
    CHECK_EXE(myConsole_CmdRegister_td(&paramCmd));

    cmdArgsMode_sts.mode_stp = arg_int1("m", "mode", "<mode>", "WIFI mode selection");
    cmdArgsMode_sts.mode_stp->ival[0] = MODE_ACCESSPOINT; // set default value
    cmdArgsMode_sts.end_stp = arg_end(2);

    const myConsole_cmd_t param2Cmd = {
        .command = "mod",
        .help = "Wifi station mode setup",
        .hint = NULL,
        .func = &CmdHandlerChangeMode_s32,
        .argtable = &cmdArgsMode_sts
    };
    CHECK_EXE(myConsole_CmdRegister_td(&param2Cmd));
}

/**---------------------------------------------------------------------------------------
 * @brief     Allocate service handle
*//*-----------------------------------------------------------------------------------*/
wifiCtrl_serviceHdl_t wifiIf_RegisterService_xp(wifiIf_service_t *service_stp)
{
    wifiCtrl_serviceHdl_t handle_xp;

    if(NULL != service_stp)
    {
        handle_xp = malloc(sizeof(wifiCtrl_serviceObj_t));
        if(NULL != handle_xp)
        {
            memset(handle_xp, 0, sizeof(wifiCtrl_serviceObj_t));
            // initialize the handle data
            memcpy(&handle_xp->service_st, service_stp, sizeof(handle_xp->service_st));
            handle_xp->active_bol = true;

            // add the handle to the list, either as first element or at the end
            AddServiceToList_vd(handle_xp);

            ESP_LOGD(TAG, "wifi service allocated");
        }
    }
    else
    {
        handle_xp = NULL;
    }

    if(NULL == handle_xp)
    {
        ESP_LOGE(TAG, "wifi service handle allocation failed...");
    }

    return(handle_xp);
}

/***************************************************************************************/
/* Local functions: */

/**---------------------------------------------------------------------------------------
 * @brief     Add the subscription object handle to the list for later handling
 * @author    S. Wink
 * @date      24. Jan. 2019
 * @param     subsHdl_xp        subscription handler
 * @return    n/a
*//*------------------------------------------------------------------------------------*/
static void AddServiceToList_vd(wifiCtrl_serviceHdl_t hdl_xp)
{
    if(NULL == this_sst.service_xp)
    {
        this_sst.service_xp = hdl_xp;
        this_sst.service_xp->next_xp = NULL;
    }
    else
    {
        wifiCtrl_serviceHdl_t current_xp = this_sst.service_xp;
        while(NULL != current_xp->next_xp)
        {
            current_xp = current_xp->next_xp;
        }
        current_xp->next_xp = hdl_xp;
    }
}

/**---------------------------------------------------------------------------------------
 * @brief     Add the subscription object handle to the list for later handling
 * @author    S. Wink
 * @date      24. Jan. 2019
 * @param     subsHdl_xp        subscription handler
 * @return    n/a
*//*------------------------------------------------------------------------------------*/
/*static void RemoveSubsFromList_vd(wifiCtrl_serviceHdl_t hdl_xp)
{
    mqttdrv_subsHdl_t last_xp = NULL;
    mqttdrv_subsHdl_t current_xp;

    if((NULL != hdl_xp) && (NULL != this_sst.service_xp))
    {
        last_xp = this_sst.service_xp;
        current_xp = this_sst.service_xp->next_xp;

        while(NULL != current_xp)
        {
            if(hdl_xp == current_xp)
            {
                // we found the object to remove
                last_xp->next_xp = current_xp->next_xp;
                free(current_xp);
                break;
            }
            last_xp = current_xp;
            current_xp = current_xp->next_xp;
        }
    }
}*/

/**--------------------------------------------------------------------------------------
 * @brief     Function to initialize WIFI to access point mode
 * @author    S. Wink
 * @date      24. Jan. 2019
*//*-----------------------------------------------------------------------------------*/
static esp_err_t Reconnect_st(void)
{
    esp_err_t result_st = ESP_FAIL;
    bool exeResult_bol = true;

    if(MODE_STATION == mode_ens)
    {
        this_sst.connectRetries_u8++;
        if(MAXIMUM_CONN_RETRIES <= this_sst.connectRetries_u8)
        {
            this_sst.Converter_fcp = wifiAp_EventConverter_u32;
            exeResult_bol &= CHECK_EXE(wifiStation_Stop_st());
            SetAndCheckState_td(STATE_WIFI_STOPPED);
            // switch to access point mode using the timer and a recall of this function
            this_sst.connectRetries_u8 = 0U;
            mode_ens = MODE_ACCESSPOINT;
            exeResult_bol &= CHECK_EXE(StartTimeout_td());
        }
        else
        {
            this_sst.Converter_fcp = wifiStation_EventConverter_u32;
            exeResult_bol &= CHECK_EXE(wifiStation_Start_st(&stationParam_sts));
            exeResult_bol &= CHECK_EXE(StartTimeout_td());
        }
    }
    else if(MODE_ACCESSPOINT == mode_ens)
    {
        this_sst.Converter_fcp = wifiAp_EventConverter_u32;
        exeResult_bol &= CHECK_EXE(wifiAp_Start_st());
    }
    else
    {
        ESP_LOGE(TAG, "unexpected wifi mode during start detected: %d", mode_ens);
        this_sst.Converter_fcp = wifiAp_EventConverter_u32;
        exeResult_bol &= CHECK_EXE(wifiAp_Start_st());
        mode_ens = DEFAULT_MODE;
    }

    if(true == exeResult_bol)
    {
        result_st = ESP_OK;
    }
    return(result_st);
}

/**---------------------------------------------------------------------------------------
 * @brief     task routine for the wifi handling
 * @author    S. Wink
 * @date      24. Jan. 2019
 * @param     pvParameters      interface variable from freertos
*//*-----------------------------------------------------------------------------------*/
static void Task_vd(void *pvParameters)
{
    EventBits_t uxBits_st;
    uint32_t bits_u32 = wifiIf_EVENT_CLIENT_CONNECTED | wifiIf_EVENT_CLIENT_DISCONNECTED
                        | wifiIf_EVENT_STATION_CONNECTED |
                        wifiIf_EVENT_STATION_DISCONNECTED |
                        wifiIf_EVENT_CONN_TIMEOUT;

    ESP_LOGD(TAG, "wifiTask started...");
    while(1)
    {
        uxBits_st = xEventGroupWaitBits(this_sst.wifiEventGroup_xp, bits_u32,
                                         true, false, portMAX_DELAY); // @suppress("Symbol is not resolved")

        if(0 != (uxBits_st & wifiIf_EVENT_CLIENT_CONNECTED))
        {
            if(NULL != this_sst.service_st.OnClientConnection_fcp)
            {
                this_sst.service_st.OnClientConnection_fcp();
                ESP_LOGI(TAG, "executed wifi ap connect callback function...");
                SetAndCheckState_td(STATE_CLIENT_CONNECTED);
            }
        }
        if(0 != (uxBits_st & wifiIf_EVENT_CLIENT_DISCONNECTED))
        {
            SetAndCheckState_td(STATE_WAITING_FOR_CLIENT);
        }

        if(0 != (uxBits_st & wifiIf_EVENT_STATION_CONNECTED))
        {
            if(NULL != this_sst.service_st.OnStationConncetion_fcp)
            {
                this_sst.service_st.OnStationConncetion_fcp();
                ESP_LOGI(TAG, "executed wifi station connect callback function...");
                xTimerStop(this_sst.timer_xp, 0);
                SetAndCheckState_td(STATE_CONNECTED);
            }
        }
        if(0 != (uxBits_st & wifiIf_EVENT_STATION_DISCONNECTED))
        {
            if(NULL != this_sst.service_st.OnDisconncetion_fcp)
            {
                this_sst.service_st.OnDisconncetion_fcp();
                ESP_LOGI(TAG, "executed wifi station disconnect callback function...");
                SetAndCheckState_td(STATE_DISCONNECTED);
            }
            CHECK_EXE(Reconnect_st());
        }
        if(0 != (uxBits_st & wifiIf_EVENT_CONN_TIMEOUT))
        {
            xTimerStop(this_sst.timer_xp, 0);
            CHECK_EXE(Reconnect_st());
        }
    }
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
    int32_t nerrors_s32 = arg_parse(argc_s32, argv, (void**) &cmdArgsStation_sts);

    if (nerrors_s32 != 0) {
        arg_print_errors(stderr, cmdArgsStation_sts.end_stp, argv[0]);
        return 1;
    }

    memset(stationParam_sts.ssid, 0, sizeof(stationParam_sts.ssid));
    memcpy(stationParam_sts.ssid, cmdArgsStation_sts.ssid_stp->sval,
            sizeof(stationParam_sts.ssid));

    memset(stationParam_sts.password, 0, sizeof(stationParam_sts.password));
    memcpy(stationParam_sts.password, cmdArgsStation_sts.password_stp->sval,
            sizeof(stationParam_sts.password));

    ESP_LOGI(TAG, "SSID: %s, PWD: %s", stationParam_sts.ssid, stationParam_sts.password);
    return(0);
}

/**--------------------------------------------------------------------------------------
 * @brief     Handler for console command parameter set
 * @author    S. Wink
 * @date      24. Jan. 2019
 * @param     argc_s32  count of argument list
 * @param     argv      pointer to argument list
 * @return    not equal to zero if error detected
*//*-----------------------------------------------------------------------------------*/
static int32_t CmdHandlerChangeMode_s32(int32_t argc_s32, char** argv)
{
    int32_t nerrors_s32 = arg_parse(argc_s32, argv, (void**) &cmdArgsMode_sts);

    if (nerrors_s32 != 0) {
        arg_print_errors(stderr, cmdArgsMode_sts.end_stp, argv[0]);
        return 1;
    }

    mode_ens = *cmdArgsMode_sts.mode_stp->ival;
    CHECK_EXE(paramif_Write_td(modeParaHdl_xps, (uint8_t *) &mode_ens));
    ESP_LOGI(TAG, "new mode received: %d", mode_ens);

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
static esp_err_t EventHandler_td(void *ctx_vp, system_event_t *event_stp)
{
    esp_err_t result_st = ESP_OK;
    uint32_t event_u32 = 0;

    event_u32 = this_sst.Converter_fcp(event_stp);

    if(wifiIf_EVENT_DONT_CARE != event_u32)
    {
        xEventGroupSetBits(this_sst.wifiEventGroup_xp, event_u32);
    }

    return(result_st);
}

/**---------------------------------------------------------------------------------------
 * @brief     callback function for the timer event handler
 * @author    S. Wink
 * @date      24. Jan. 2019
 * @param     xTimer      handle to timer
*//*-----------------------------------------------------------------------------------*/
static void HandleConnectionTimeout_vd(TimerHandle_t xTimer)
{
    ESP_LOGD(TAG, "timer callback...");
    xEventGroupSetBits(this_sst.wifiEventGroup_xp, wifiIf_EVENT_CONN_TIMEOUT);
}

/**---------------------------------------------------------------------------------------
 * @brief     callback function for the timer event handler
 * @author    S. Wink
 * @date      24. Jan. 2019
 * @param     xTimer      handle to timer
*//*-----------------------------------------------------------------------------------*/
static esp_err_t SetAndCheckState_td(objectState_t state_en)
{
    esp_err_t result_st = ESP_OK;
    ESP_LOGD(TAG, "state change: %d", state_en);
    this_sst.state_en = state_en;
    return(result_st);
}

/**---------------------------------------------------------------------------------------
 * @brief     starts the timer if it is proper allicated
 * @author    S. Wink
 * @date      24. Jan. 2019
 * @return    ESP_OK, ESP_FAIL
*//*-----------------------------------------------------------------------------------*/
static esp_err_t StartTimeout_td(void)
{
    esp_err_t exeResult_st = ESP_FAIL;

    if(NULL != this_sst.timer_xp)
    {
        xTimerStart(this_sst.timer_xp, 0);
        exeResult_st = ESP_OK;
    }

    return(exeResult_st);
}

/**---------------------------------------------------------------------------------------
 * @brief     creates the timer object
 * @author    S. Wink
 * @date      26. Jan. 2020
 * @return    ESP_OK if creation was successful, else ESP_FAIL
*//*-----------------------------------------------------------------------------------*/
static esp_err_t CreateTimer_td(void)
{
    esp_err_t exeResult_td = ESP_FAIL;

    this_sst.timer_xp = xTimerCreate("WIFI_Timeout", pdMS_TO_TICKS(5000), false,
                                                (void *) 0, HandleConnectionTimeout_vd);
    if(NULL != this_sst.timer_xp)
    {
        exeResult_td = ESP_OK;
    }   
    return(exeResult_td); 
}

/**---------------------------------------------------------------------------------------
 * @brief     creates the event group object
 * @author    S. Wink
 * @date      26. Jan. 2020
 * @return    ESP_OK if creation was successful, else ESP_FAIL
*//*-----------------------------------------------------------------------------------*/
static esp_err_t CreateEventGroup_td(void)
{
    esp_err_t exeResult_td = ESP_FAIL;

    this_sst.wifiEventGroup_xp = xEventGroupCreate();
    if(NULL != this_sst.wifiEventGroup_xp)
    {
        exeResult_td = ESP_OK;
    }   
    return(exeResult_td); 
}

/**---------------------------------------------------------------------------------------
 * @brief     creates the task object
 * @author    S. Wink
 * @date      26. Jan. 2020
 * @return    ESP_OK if creation was successful, else ESP_FAIL
*//*-----------------------------------------------------------------------------------*/
static esp_err_t CreateTask_td(void)
{
    esp_err_t exeResult_td = ESP_FAIL;

    (void)xTaskCreate(Task_vd, "wifiTask", 4096, NULL, 4, &this_sst.task_xp);
    if(NULL != this_sst.task_xp)
    {
        exeResult_td = ESP_OK;
    }   
    return(exeResult_td); 
}

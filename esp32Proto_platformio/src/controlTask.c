/*****************************************************************************************
* FILENAME :        controlTask.c
*
* DESCRIPTION :
*       This module is the startpoint for my specific application
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
#include "controlTask.h"

#include "secrets.h"

#include <mqttdrv.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"

#include "consoleSocket.h"
#include "myConsole.h"
#include "paramif.h"
#include "otaUpdate.h"
#include "sdkconfig.h"

#include "udpLog.h"
#include "utils.h"

#include "wifiIf.h"
#include "wifiCtrl.h"
#include "appIdent.h"
#include "devmgr.h"
#include "logcfg.h"

/****************************************************************************************/
/* Local constant defines */

#define MODULE_TAG "controlTask"
#define CHECK_EXE(arg) utils_CheckAndLogExecution_vd(MODULE_TAG, arg, __LINE__)

/****************************************************************************************/
/* Local function like makros */

/****************************************************************************************/
/* Local type definitions (enum, struct, union) */
typedef struct ctrlData_tag
{
    uint32_t startupCounter_u32;
}ctrlData_t;

/****************************************************************************************/
/* Local functions prototypes: */
static void Task_vd(void *param_vdp);
static void RegisterCommands_vd(void);
static int CommandRebootHandler_i(int argc, char** argv);
static void ServiceCbWifiStationConn_vd(void);
static void ServiceCbWifiApClientConn_vd(void);
static void ServiceCbWifiDisconnected_vd(void);
static void StartFullService_vd(void);
static void StartSelfServicesOnly_vd(void);
static void SocketErrorCb_vd(void);

static void InitializeParameterHandling_vd(void);
static void StartupAndApplicationIdent_vd(void);
static void InitializeWifi_vd(void);
static void InitializeBasicWifiServices_vd(void);

/****************************************************************************************/
/* Local variables: */

const int WIFI_STATION      = BIT0;
const int WIFI_AP_CLIENT    = BIT1;
const int WIFI_DISCONN      = BIT2;
const int SOCKET_ERROR      = BIT3;
const int SYSTEM_REBOOT     = BIT4;


static EventGroupHandle_t controlEventGroup_sts;
static const char *TAG = MODULE_TAG;

//static void InitializeCommands(void);
static ctrlData_t controlData_sts =
{
    .startupCounter_u32 = 0UL,
};

static const ctrlData_t defaultControlData_stsc =
{
    .startupCounter_u32 = 0UL,
};

static const char *MQTT_HOST = secrets_MQTT_SERVER;
static const uint32_t mqttPort_u32sc = secrets_MQTT_PORT;
static const char *MQTT_USER_NAME = secrets_MQTT_USER_NAME;
static const char *MQTT_PASSWORD = secrets_MQTT_PASSWORD;


static const char *CTRL_PARA_IDENT = "controls";

/****************************************************************************************/
/* Global functions (unlimited visibility) */

/**---------------------------------------------------------------------------------------
 * @brief     Start function for control task
*//*-----------------------------------------------------------------------------------*/
esp_err_t controlTask_StartSystem_td(void)
{
    esp_err_t success_st = ESP_OK;
    myConsole_config_t consoleConfig_st =
    {
            .max_cmdline_args = 8,
            .max_cmdline_length = 2048,
    };
    devmgr_param_t devMgrParam_st;

    CHECK_EXE(logcfg_Configure_st(logcfg_WIFI));

    InitializeParameterHandling_vd();

    /* initialize console object for message processing */
    CHECK_EXE(myConsole_Init_td(&consoleConfig_st));
    myConsole_RegisterHelpCommand();
    RegisterCommands_vd();

    StartupAndApplicationIdent_vd();

    /* setup event group for event receiving from other tasks and processes */
    controlEventGroup_sts = xEventGroupCreate();

    InitializeWifi_vd();

    InitializeBasicWifiServices_vd();

    /* initialize device manager */
    ESP_LOGI(TAG, "generate devices...");
    CHECK_EXE(devmgr_InitializeParameter_td(&devMgrParam_st));
    CHECK_EXE(devmgr_Initialize_td(&devMgrParam_st));
    CHECK_EXE(devmgr_GenerateDevices_td());

    /* start the control task */
    xTaskCreate(Task_vd, "controlTask", 4096, NULL, 5, NULL);
    return(success_st);
}

/****************************************************************************************/
/* Local functions: */

/**--------------------------------------------------------------------------------------
 * @brief     task routine for the control handling
 * @author    S. Wink
 * @date      24. Jan. 2019
 * @param     pvParameters      interface variable from freertos
*//*-----------------------------------------------------------------------------------*/
static void Task_vd(void *param_vdp)
{
    EventBits_t uxBits_st;
    uint32_t bits_u32 = WIFI_STATION | WIFI_AP_CLIENT | WIFI_DISCONN |
                        SOCKET_ERROR | SYSTEM_REBOOT;

    ESP_LOGI(TAG, "controlTask started...");
    while(1)
    {
        uxBits_st = xEventGroupWaitBits(controlEventGroup_sts, bits_u32,
                                         true, false, portMAX_DELAY); // @suppress("Symbol is not resolved")

        if(0 != (uxBits_st & WIFI_STATION))
        {
            StartFullService_vd();
        }

        if(0 != (uxBits_st & WIFI_AP_CLIENT))
        {
            StartSelfServicesOnly_vd();   
        }

        if(0 != (uxBits_st & WIFI_DISCONN))
        {
            ESP_LOGI(TAG, "WIFI_DISCONNECTED received...");
            //consoleSocket_Deactivate_vd();
            //udpLog_Free_vd();
        }

        if(0 != (uxBits_st & SOCKET_ERROR))
        {
            ESP_LOGE(TAG, "SOCKET_ERROR received...");
        }

        if(0 != (uxBits_st & SYSTEM_REBOOT))
        {
            vTaskDelay(5000 / portTICK_RATE_MS);
            esp_restart();
        }
    }
}

/**---------------------------------------------------------------------------------------
 * @brief     Registration of the supported console commands
 * @author    S. Wink
 * @date      24. Jan. 2019
*//*-----------------------------------------------------------------------------------*/
static void RegisterCommands_vd(void)
{
    const myConsole_cmd_t rebootCommand_stc =
    {
        .command = "boot",
        .help = "Reboot the controller",
        .func = &CommandRebootHandler_i,
    };

    ESP_ERROR_CHECK(myConsole_CmdRegister_td(&rebootCommand_stc));
}

/**---------------------------------------------------------------------------------------
 * @brief     Handler for console command to reboot the system
 * @author    S. Wink
 * @date      08. Mar. 2019
 * @param     argc  count of argument list
 * @param     argv  pointer to argument list
 * @return    not equal to zero if error detected
*//*-----------------------------------------------------------------------------------*/
static int CommandRebootHandler_i(int argc, char** argv)
{
    ESP_LOGI(TAG, "networkTask: Reboot in 2 seconds...");
    xEventGroupSetBits(controlEventGroup_sts, SYSTEM_REBOOT);
    return(0);
}

/**---------------------------------------------------------------------------------------
 * @brief     Service callback function, when connected to a wifi station
 * @author    S. Wink
 * @date      06. Sep. 2019
*//*-----------------------------------------------------------------------------------*/
static void ServiceCbWifiStationConn_vd(void)
{
    ESP_LOGI(TAG, "callback ServiceCbWifiStationConn_vd...");
    xEventGroupSetBits(controlEventGroup_sts, WIFI_STATION);
}

/**---------------------------------------------------------------------------------------
 * @brief     Service callback function, when in AP mode a client is connected
 * @author    S. Wink
 * @date      06. Sep. 2019
*//*-----------------------------------------------------------------------------------*/
static void ServiceCbWifiApClientConn_vd(void)
{
    ESP_LOGI(TAG, "callback ServiceCbWifiApClientConn_vd...");
    xEventGroupSetBits(controlEventGroup_sts, WIFI_AP_CLIENT);
}

/**---------------------------------------------------------------------------------------
 * @brief     Service callback function, when wifi is disconnected
 * @author    S. Wink
 * @date      06. Sep. 2019
*//*-----------------------------------------------------------------------------------*/
static void ServiceCbWifiDisconnected_vd(void)
{
    ESP_LOGI(TAG, "callback ServiceCbWifiDisconnected_vd...");
    xEventGroupSetBits(controlEventGroup_sts, WIFI_DISCONN);
    consoleSocket_Deactivate_vd();
    udpLog_Free_st();
}

/**---------------------------------------------------------------------------------------
 * @brief     Starts all services when we are connected to an access point.
 * @author    S. Wink
 * @date      25. Jan. 2020
*//*-----------------------------------------------------------------------------------*/
static void StartFullService_vd(void)
{
    udpLog_param_t logServer_st;

    ESP_LOGI(TAG, "WIFI_STATION received...");
    consoleSocket_Activate_vd();

    CHECK_EXE(udpLog_InitializeParameter_st(&logServer_st));
    logServer_st.ipAddr_cchp = "192.168.178.89";
    logServer_st.conPort_u32 = 1337;
    CHECK_EXE(udpLog_Initialize_st(&logServer_st));

    mqttdrv_StartMqttDemon_vd();
}

/**---------------------------------------------------------------------------------------
 * @brief     This function only starts services which are available if the device is
 *              not connected to an access point and opened its own access point for 
 *              maintenence and support.
 * @author    S. Wink
 * @date      25. Jan. 2020
*//*-----------------------------------------------------------------------------------*/
static void StartSelfServicesOnly_vd(void)
{
    udpLog_param_t logServer_st;

    ESP_LOGI(TAG, "WIFI_AP_CLIENT received...");
    consoleSocket_Activate_vd();
    
    CHECK_EXE(udpLog_InitializeParameter_st(&logServer_st));
    logServer_st.ipAddr_cchp = "192.168.4.2";
    logServer_st.conPort_u32 = 1337;
    CHECK_EXE(udpLog_Initialize_st(&logServer_st));
}

/**---------------------------------------------------------------------------------------
 * @brief     Callback function to notify task that socket run to an error
 * @author    S. Wink
 * @date      24. Jan. 2019
*//*-----------------------------------------------------------------------------------*/
static void SocketErrorCb_vd(void)
{
    ESP_LOGI(TAG, "callback SocketErrorCb_vd...");
    xEventGroupSetBits(controlEventGroup_sts, SOCKET_ERROR);
}

/**---------------------------------------------------------------------------------------
 * @brief     Initialize and configer the parameter handling
 * @author    S. Wink
 * @date      25. Jan. 2020
*//*-----------------------------------------------------------------------------------*/
static void InitializeParameterHandling_vd(void)
{
    paramif_param_t paramHdl_st;

    CHECK_EXE(paramif_InitializeParameter_td(&paramHdl_st));
    CHECK_EXE(paramif_Initialize_td(&paramHdl_st));
}

/**---------------------------------------------------------------------------------------
 * @brief     print and update startup counter and print firmware identifications
 * @author    S. Wink
 * @date      25. Jan. 2020
*//*-----------------------------------------------------------------------------------*/
static void StartupAndApplicationIdent_vd(void)
{
    paramif_allocParam_t controlAllocParam_st;
    static paramif_objHdl_t ctrlParaHdl_xp;

    /* initialize and register Version information */
    CHECK_EXE(appIdent_Initialize_st());
    appIdent_LogFirmwareIdent_vd(TAG);

    CHECK_EXE(paramif_InitializeAllocParameter_td(&controlAllocParam_st));
    controlAllocParam_st.length_u16 = sizeof(ctrlData_t);
    controlAllocParam_st.defaults_u8p = (uint8_t *)&defaultControlData_stsc;
    controlAllocParam_st.nvsIdent_cp = CTRL_PARA_IDENT;
    ctrlParaHdl_xp = paramif_Allocate_stp(&controlAllocParam_st);

    /* update startup counter in none volatile memory */
    CHECK_EXE(paramif_Read_td(ctrlParaHdl_xp, (uint8_t *) &controlData_sts));
    controlData_sts.startupCounter_u32++;
    CHECK_EXE(paramif_Write_td(ctrlParaHdl_xp, (uint8_t *) &controlData_sts));
    ESP_LOGI(TAG, "New startup detected, system restarted %d times.",
                controlData_sts.startupCounter_u32);
    ESP_LOGI(TAG, "----------------------------------------------------");
}

/**---------------------------------------------------------------------------------------
 * @brief     Initialze WIFI service
 * @author    S. Wink
 * @date      25. Jan. 2020
*//*-----------------------------------------------------------------------------------*/
static void InitializeWifi_vd(void)
{
    wifiIf_service_t services_st =
    {
        ServiceCbWifiStationConn_vd,
        ServiceCbWifiDisconnected_vd,
        ServiceCbWifiApClientConn_vd
    };

    CHECK_EXE(wifiCtrl_Initialize_st(&services_st));
    CHECK_EXE(wifiCtrl_Start_st());
    wifiCtrl_RegisterWifiCommands();
}

/**---------------------------------------------------------------------------------------
 * @brief     Initialze WIFI service
 * @author    S. Wink
 * @date      25. Jan. 2020
*//*-----------------------------------------------------------------------------------*/
static void InitializeBasicWifiServices_vd(void)
{
    socketServer_parameter_t sockParam_st =
    {
        SocketErrorCb_vd
    };
    otaUpdate_param_t otaParam_st;
    mqttdrv_param_t mqttParam_st;

    consoleSocket_Initialize_st(&sockParam_st);

    /* initialize over the air firmware update */
    otaUpdate_InitializeParameter_td(&otaParam_st);
    otaUpdate_Initialize_td(&otaParam_st);

    /* initialize the mqtt driver including the mqtt client */
    CHECK_EXE(mqttdrv_InitializeParameter_td(&mqttParam_st));
    memcpy(mqttParam_st.host_u8a, MQTT_HOST, strlen(MQTT_HOST));
    mqttParam_st.port_u32 = mqttPort_u32sc;
    memcpy(mqttParam_st.userName_u8a, MQTT_USER_NAME, strlen(MQTT_USER_NAME));
    memcpy(mqttParam_st.userPwd_u8a, MQTT_PASSWORD, strlen(MQTT_PASSWORD));
    CHECK_EXE(mqttdrv_Initialize_td(&mqttParam_st));
}






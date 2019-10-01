/*****************************************************************************************
* FILENAME :        controlTask.c
*
* DESCRIPTION :
*       This module
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

/****************************************************************************************/
/* Local constant defines */
#ifndef BIT0
    #define BIT0    0x00000000
    #define BIT1    0x00000001
    #define BIT2    0x00000002
    #define BIT3    0x00000004
    #define BIT4    0x00000008
#endif

#define MQTT_SERVICE_ACTIVE 1
#define MQTT_DEVICES_ACTIVE 1

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
static void RegisterCommands(void);
static int CommandInfoHandler_i(int argc, char** argv);
static int CommandRebootHandler_i(int argc, char** argv);
static void ServiceCbWifiStationConn(void);
static void ServiceCbWifiApClientConn(void);
static void ServiceCbWifiDisconnected(void);
static void PrintFirmwareIdent(void);

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

static const char *MQTT_HOST = "192.168.178.45";
static const uint32_t mqttPort_u32sc = 1883;
static const char *MQTT_USER_NAME = "winkste";
static const char *MQTT_PASSWORD = "sw10950";


static const char *CTRL_PARA_IDENT = "controls";
static paramif_objHdl_t ctrlParaHdl_xps;

static mqttdrv_param_t mqttParam_st;
static devmgr_param_t devMgrParam_st;
/****************************************************************************************/
/* Global functions (unlimited visibility) */

/**---------------------------------------------------------------------------------------
 * @brief     Initialization function for control task
*//*-----------------------------------------------------------------------------------*/
esp_err_t controlTask_Initialize_st(void)
{
    esp_err_t success_st = ESP_OK;

    socketServer_parameter_t sockParam_st =
    {
        controlTask_SetEventSocketError
    };

    myConsole_config_t consoleConfig_st =
    {
            .max_cmdline_args = 8,
            .max_cmdline_length = 2048,
    };
    paramif_param_t paramHdl_st;
    paramif_allocParam_t controlAllocParam_st;
    otaUpdate_param_t otaParam_st;

    /* parameter initialization */
    CHECK_EXE(paramif_InitializeParameter_td(&paramHdl_st));
    CHECK_EXE(paramif_Initialize_td(&paramHdl_st));
    CHECK_EXE(paramif_InitializeAllocParameter_td(&controlAllocParam_st));
    controlAllocParam_st.length_u16 = sizeof(ctrlData_t);
    controlAllocParam_st.defaults_u8p = (uint8_t *)&defaultControlData_stsc;
    controlAllocParam_st.nvsIdent_cp = CTRL_PARA_IDENT;
    ctrlParaHdl_xps = paramif_Allocate_stp(&controlAllocParam_st);

    /* initialize console object for message processing */
    CHECK_EXE(myConsole_Init_td(&consoleConfig_st));
    myConsole_RegisterHelpCommand();
    RegisterCommands();

    /* initialize and register Version information */
    CHECK_EXE(appIdent_Initialize_st());
    PrintFirmwareIdent();


    /* setup event group for event receiving from other tasks and processes */
    controlEventGroup_sts = xEventGroupCreate();

    wifiIf_service_t services_st =
    {
        ServiceCbWifiStationConn,
        ServiceCbWifiDisconnected,
        ServiceCbWifiApClientConn
    };
    wifiCtrl_Initialize_st(&services_st);
    wifiCtrl_Start_st();
    wifiCtrl_RegisterWifiCommands();

    consoleSocket_Initialize_st(&sockParam_st);


    /* update startup counter in none volatile memory */
    CHECK_EXE(paramif_Read_td(ctrlParaHdl_xps, (uint8_t *) &controlData_sts));
    controlData_sts.startupCounter_u32++;
    CHECK_EXE(paramif_Write_td(ctrlParaHdl_xps, (uint8_t *) &controlData_sts));
    ESP_LOGI(TAG, "New startup detected, system restarted %d times.",
                controlData_sts.startupCounter_u32);

    /* initialize over the air firmware update */
    otaUpdate_InitializeParameter_td(&otaParam_st);
    otaUpdate_Initialize_td(&otaParam_st);

#ifdef MQTT_SERVICE_ACTIVE
    /* initialize the mqtt driver including the mqtt client */
    CHECK_EXE(mqttdrv_InitializeParameter(&mqttParam_st));
    memcpy(mqttParam_st.host_u8a, MQTT_HOST, strlen(MQTT_HOST));
    mqttParam_st.port_u32 = mqttPort_u32sc;
    memcpy(mqttParam_st.userName_u8a, MQTT_USER_NAME, strlen(MQTT_USER_NAME));
    memcpy(mqttParam_st.userPwd_u8a, MQTT_PASSWORD, strlen(MQTT_PASSWORD));
    CHECK_EXE(mqttdrv_Initialize(&mqttParam_st));

#endif

#ifdef MQTT_DEVICES_ACTIVE
    /* initialize device manager */
    CHECK_EXE(devmgr_InitializeParameter(&devMgrParam_st));
    CHECK_EXE(devmgr_Initialize(&devMgrParam_st));
    ESP_LOGI(TAG, "generate devices...");
    devmgr_GenerateDevices();
#endif

    /* start the control task */
    xTaskCreate(controlTask_Task_vd, "controlTask", 4096, NULL, 5, NULL);
    return(success_st);
}

/**---------------------------------------------------------------------------------------
 * @brief     Callback function to notify task that socket run to an error
*//*-----------------------------------------------------------------------------------*/
void controlTask_SetEventSocketError(void)
{
    ESP_LOGI(TAG, "callback controlTask_SetEventSocketError...");
    xEventGroupSetBits(controlEventGroup_sts, SOCKET_ERROR);
}

/**---------------------------------------------------------------------------------------
 * @brief     task routine for the control handling
*//*-----------------------------------------------------------------------------------*/
void controlTask_Task_vd(void *pvParameters)
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
            ESP_LOGI(TAG, "WIFI_STATION received...");
            consoleSocket_Activate_vd();
            //udpLog_Init_st( "192.168.178.25", 1337);
            mqttdrv_StartMqttDemon();

        }

        if(0 != (uxBits_st & WIFI_AP_CLIENT))
        {
            ESP_LOGI(TAG, "WIFI_AP_CLIENT received...");
            consoleSocket_Activate_vd();
            //udpLog_Init_st( "192.168.178.25", 1337);
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

/****************************************************************************************/
/* Local functions: */

/**---------------------------------------------------------------------------------------
 * @brief     Registration of the supported console commands
 * @author    S. Wink
 * @date      24. Jan. 2019
*//*-----------------------------------------------------------------------------------*/
static void RegisterCommands(void)
{
    const myConsole_cmd_t infoCmd_stc =
    {
        .command = "info",
        .help = "Get control task information log",
        .func = &CommandInfoHandler_i,
    };

    const myConsole_cmd_t rebootCommand_stc =
    {
        .command = "boot",
        .help = "Reboot the controller",
        .func = &CommandRebootHandler_i,
    };

    ESP_ERROR_CHECK(myConsole_CmdRegister_td(&infoCmd_stc));
    ESP_ERROR_CHECK(myConsole_CmdRegister_td(&rebootCommand_stc));
}

/**---------------------------------------------------------------------------------------
 * @brief     Handler for console command to printout control information
 * @author    S. Wink
 * @date      24. Jan. 2019
 * @param     argc  count of argument list
 * @param     argv  pointer to argument list
 * @return    not equal to zero if error detected
*//*-----------------------------------------------------------------------------------*/
static int CommandInfoHandler_i(int argc, char** argv)
{
    ESP_LOGI(TAG, "information request command received");
    PrintFirmwareIdent();
    return(0);
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
 * @param     argc  count of argument list
 * @param     argv  pointer to argument list
 * @return    not equal to zero if error detected
*//*-----------------------------------------------------------------------------------*/
static void ServiceCbWifiStationConn(void)
{
    ESP_LOGI(TAG, "callback ServiceCbWifiStationConn...");
    xEventGroupSetBits(controlEventGroup_sts, WIFI_STATION);
}

/**---------------------------------------------------------------------------------------
 * @brief     Service callback function, when in AP mode a client is connected
 * @author    S. Wink
 * @date      06. Sep. 2019
 * @param     argc  count of argument list
 * @param     argv  pointer to argument list
 * @return    not equal to zero if error detected
*//*-----------------------------------------------------------------------------------*/
static void ServiceCbWifiApClientConn(void)
{
    ESP_LOGI(TAG, "callback ServiceCbWifiApClientConn...");
    xEventGroupSetBits(controlEventGroup_sts, WIFI_AP_CLIENT);
}

/**---------------------------------------------------------------------------------------
 * @brief     Service callback function, when wifi is disconnected
 * @author    S. Wink
 * @date      06. Sep. 2019
 * @param     argc  count of argument list
 * @param     argv  pointer to argument list
 * @return    not equal to zero if error detected
*//*-----------------------------------------------------------------------------------*/
static void ServiceCbWifiDisconnected(void)
{
    ESP_LOGI(TAG, "callback ServiceCbWifiDisconnected...");
    xEventGroupSetBits(controlEventGroup_sts, WIFI_DISCONN);
}

/**---------------------------------------------------------------------------------------
 * @brief     Print the firmware identification to serial
 * @author    S. Wink
 * @date      06. Sep. 2019
*//*-----------------------------------------------------------------------------------*/
static void PrintFirmwareIdent(void)
{
    ESP_LOGI(TAG, "----------------------------------------------------");
    ESP_LOGI(TAG, "Firmware PN: %s", appIdent_GetFwIdentifier_cch());
    ESP_LOGI(TAG, "Firmware Version: %s", appIdent_GetFwVersion_cch());
    ESP_LOGI(TAG, "Firmware Desc: %s", appIdent_GetFwDescription_cch());
    ESP_LOGI(TAG, "----------------------------------------------------");
    if(NULL != ctrlParaHdl_xps)
    {
        ESP_ERROR_CHECK(paramif_Read_td(ctrlParaHdl_xps, (uint8_t *) &controlData_sts));
        ESP_LOGI(TAG, "startups detected: %d", controlData_sts.startupCounter_u32);
    }
    else
    {
        ESP_LOGE(TAG, "startup counter not readable...");
    }
    ESP_LOGI(TAG, "----------------------------------------------------");
}



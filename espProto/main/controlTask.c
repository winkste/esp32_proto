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

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"

#include "myWifi.h"
#include "socketServer.h"
#include "myConsole.h"
#include "paramif.h"
#include "myVersion.h"
#include "otaUpdate.h"
#include "sdkconfig.h"

#include "../components/udpLog/include/udpLog.h"

/****************************************************************************************/
/* Local constant defines */
#ifndef BIT0
    #define BIT0    0x00000000
    #define BIT1    0x00000001
    #define BIT2    0x00000002
    #define BIT3    0x00000004
#endif

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

/****************************************************************************************/
/* Local variables: */

const int WIFI_STARTED      = BIT0;
const int WIFI_DISCONNECTED = BIT1;
const int SOCKET_ERROR      = BIT2;
const int SYSTEM_REBOOT     = BIT3;

static EventGroupHandle_t controlEventGroup_sts;
static const char *TAG = "controlTask";

//static void InitializeCommands(void);
static ctrlData_t controlData_sts =
{
    .startupCounter_u32 = 0UL,
};

static const ctrlData_t defaultControlData_stsc =
{
    .startupCounter_u32 = 0UL,
};

static const char *CTRL_PARA_IDENT = "controls";
static paramif_objHdl_t ctrlParaHdl_xps;
/****************************************************************************************/
/* Global functions (unlimited visibility) */

/**---------------------------------------------------------------------------------------
 * @brief     Initialization function for control task
*//*-----------------------------------------------------------------------------------*/
esp_err_t controlTask_Initialize_st(void)
{
    esp_err_t success_st = ESP_OK;
    myWifi_parameter_t param_st =
    {
        controlTask_SetEventWifiStarted,
        controlTask_SetEventWifiDisconnected
    };
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
    ESP_ERROR_CHECK(paramif_InitializeParameter_td(&paramHdl_st));
    ESP_ERROR_CHECK(paramif_Initialize_td(&paramHdl_st));
    ESP_ERROR_CHECK(paramif_InitializeAllocParameter_td(&controlAllocParam_st));
    controlAllocParam_st.length_u16 = sizeof(ctrlData_t);
    controlAllocParam_st.defaults_u8p = (uint8_t *)&defaultControlData_stsc;
    controlAllocParam_st.nvsIdent_cp = CTRL_PARA_IDENT;
    ctrlParaHdl_xps = paramif_Allocate_stp(&controlAllocParam_st);

    /* initialize console object for message processing */
    ESP_ERROR_CHECK(myConsole_Init_td(&consoleConfig_st));
    myConsole_RegisterHelpCommand();
    RegisterCommands();

    /* initialize and register Version information */
    ESP_ERROR_CHECK(myVersion_Initialize_st());

    /* setup event group for event receiving from other tasks and processes */
    controlEventGroup_sts = xEventGroupCreate();

    /* initialize wifi and socket server */
    myWifi_InitializeWifi_vd(&param_st);    // 1. setup wifi in general
    myWifi_InitializeWifiSta_vd();          // 2. start wifi in stationary mode
    //myWifi_InitializeWifiSoftAp_vd();     // or 2. start wifi in access point mode
    socketServer_Initialize_st(&sockParam_st);  // 3. start the socket sever
    myWifi_RegisterWifiCommands();              // 4. register wifi related commands

    paramif_PrintHandle_vd(ctrlParaHdl_xps);
    /* update startup counter in none volatile memory */
    ESP_ERROR_CHECK(paramif_Read_td(ctrlParaHdl_xps, (uint8_t *) &controlData_sts));
    controlData_sts.startupCounter_u32++;
    ESP_ERROR_CHECK(paramif_Write_td(ctrlParaHdl_xps, (uint8_t *) &controlData_sts));
    ESP_LOGI(TAG, "New startup detected, system restarted %d times.",
                controlData_sts.startupCounter_u32);

    otaUpdate_InitializeParameter_td(&otaParam_st);
    otaUpdate_Initialize_td(&otaParam_st);

    /* start the control task */
    xTaskCreate(controlTask_Task_vd, "controlTask", 4096, NULL, 5, NULL);
    return(success_st);
}

/**---------------------------------------------------------------------------------------
 * @brief     CallBack function to notify controlTask that wifi is active
*//*-----------------------------------------------------------------------------------*/
void controlTask_SetEventWifiStarted(void)
{
    ESP_LOGI(TAG, "callback controlTask_SetEventWifiStarted...");
    xEventGroupSetBits(controlEventGroup_sts, WIFI_STARTED);
}

/**---------------------------------------------------------------------------------------
 * @brief     Callback function to notify controlTask that WIFI is disconnected
*//*-----------------------------------------------------------------------------------*/
void controlTask_SetEventWifiDisconnected(void)
{
    ESP_LOGI(TAG, "callback controlTask_SetEventWifiDisconnected...");
    xEventGroupSetBits(controlEventGroup_sts, WIFI_DISCONNECTED);
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
    uint32_t bits_u32 = WIFI_STARTED | WIFI_DISCONNECTED | SOCKET_ERROR | SYSTEM_REBOOT;

    ESP_LOGI(TAG, "controlTask started...");
    while(1)
    {
        uxBits_st = xEventGroupWaitBits(controlEventGroup_sts, bits_u32,
                                         true, false, portMAX_DELAY); // @suppress("Symbol is not resolved")

        if(0 != (uxBits_st & WIFI_STARTED))
        {
            ESP_LOGI(TAG, "WIFI_STARTED received...");
            // activate socket server
            socketServer_Activate_vd();
           udpLog_Init_st( "192.168.178.25", 1337);
        }

        if(0 != (uxBits_st & WIFI_DISCONNECTED))
        {
            ESP_LOGI(TAG, "WIFI_DISCONNECTED received...");
        }

        if(0 != (uxBits_st & SOCKET_ERROR))
        {
            ESP_LOGE(TAG, "SOCKET_ERROR received...");
        }

        if(0 != (uxBits_st & SYSTEM_REBOOT))
        {
            vTaskDelay(2000 / portTICK_RATE_MS);
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
    ESP_ERROR_CHECK(paramif_Read_td(ctrlParaHdl_xps, (uint8_t *) &controlData_sts));
    ESP_LOGI(TAG, "startups detected: %d", controlData_sts.startupCounter_u32);
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

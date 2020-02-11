/*****************************************************************************************
* FILENAME :        devmgr.c
*
* SHORT DESCRIPTION:
*   Implementation file for devmgr module.
*
* DETAILED DESCRIPTION :
*       
*
* AUTHOR :    Stephan Wink        CREATED ON :    24. Jan. 2020
*
* Copyright (c) [2020] [Stephan Wink]
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
*****************************************************************************************/

/****************************************************************************************/
/* Include Interfaces */
#include "devmgr.h"

#include "stdint.h"
#include "string.h"
#include "esp_err.h"
#include "esp_log.h"
#include "stdbool.h"

#include "utils.h"
#include "mqttif.h"
#include "mqttdrv.h"
#include "paramif.h"

#include "argtable3/argtable3.h"
#include "myConsole.h"

#include "gendev.h"
#include "mijasens.h"

/****************************************************************************************/
/* Local constant defines */
#define MODULE_TAG                      "devmgr"
#define DEVICE_LOCATION_STR_LENGTH      20      

/****************************************************************************************/
/* Local function like makros */

#define CHECK_EXE(arg) utils_CheckAndLogExecution_bol(MODULE_TAG, arg, __LINE__)

/****************************************************************************************/
/* Local type definitions (enum, struct, union) */

typedef enum objectState_tag
{
     STATE_NOT_INITIALIZED,
     STATE_INITIALIZED,
     STATE_DEVICES_ACTIVE
}objectState_t;

typedef enum capType_tag
{
    CAPABILITY_DEFAULT   = 0,
    CAPABILITY_DHT22,
    CAPABILITY_MIJA_SENS,
    CAPABILITY_UNDEFINED = 0xFF
}capType_t;

typedef enum devId_tag
{
    DEVICE_ID_NEW =     0,
    DEVICE_ID_DEFAULT = 98
}devId_t;

typedef struct objectParam_tag
{
    capType_t cap_en;
    devId_t devId_en;
    char devLoc_cha[DEVICE_LOCATION_STR_LENGTH];
}objectParam_t;

typedef struct objectData_tag
{
     capType_t cap_en;
     objectState_t state_st;
     char *devName_chp;
     char *id_chp;
     objectParam_t para_st;
}objectData_t;

static struct
{
    struct arg_int *cap_stp;
    struct arg_int *id_stp;
    struct arg_str *location_stp;
    struct arg_end *end_stp;
}cmdArgsDevice_sts;

/****************************************************************************************/
/* Local functions prototypes: */
static esp_err_t GenerateDefaultDevice_st(void);
static esp_err_t GenerateMijaDevice_st(void);
static int32_t CmdHandlerChangeParameter_s32(int32_t argc_s32, char** argv);
static void RegisterDeviceCommands_vd(void);

/****************************************************************************************/
/* Local variables: */
static objectData_t this_sst =
{
    .cap_en = CAPABILITY_DEFAULT,
    .state_st = STATE_NOT_INITIALIZED,
    .devName_chp = "dev98",
    .id_chp = "chan1"
};

static const objectParam_t DEFAULT_PARA = 
{
    .cap_en = CAPABILITY_DEFAULT,
    .devId_en = DEVICE_ID_DEFAULT,
    .devLoc_cha = "default",
};
static const char *MODE_PARA_IDENT = "devMgr";
paramif_objHdl_t deviceParaHdl_xps;

static const char *TAG = MODULE_TAG;

/****************************************************************************************/
/* Global functions (unlimited visibility) */

/**--------------------------------------------------------------------------------------
 * @brief     Initializes the initialization structure of the device manager module
*//*-----------------------------------------------------------------------------------*/
extern esp_err_t devmgr_InitializeParameter_td(devmgr_param_t *param_stp)
{
    esp_err_t result_st = ESP_OK;

    return(result_st);
}

/**--------------------------------------------------------------------------------------
 * @brief     Initialization of the device manager module
*//*-----------------------------------------------------------------------------------*/
extern esp_err_t devmgr_Initialize_td(devmgr_param_t *param_stp)
{
    esp_err_t result_st = ESP_FAIL;
    bool exeResult_bol = true;
    paramif_allocParam_t deviceAllocParam_st;

    exeResult_bol &= CHECK_EXE(paramif_InitializeAllocParameter_td(
                                                &deviceAllocParam_st));
    deviceAllocParam_st.length_u16 = sizeof(this_sst.para_st);
    deviceAllocParam_st.defaults_u8p = (uint8_t *)&DEFAULT_PARA;
    deviceAllocParam_st.nvsIdent_cp = MODE_PARA_IDENT;
    deviceParaHdl_xps = paramif_Allocate_stp(&deviceAllocParam_st);
    paramif_PrintHandle_vd(deviceParaHdl_xps);
    exeResult_bol &= CHECK_EXE(paramif_Read_td(deviceParaHdl_xps, 
                                                (uint8_t *) &this_sst.para_st));
    ESP_LOGI(TAG, "loaded device parameter from flash... ");
    ESP_LOGI(TAG, "device capability: %d", this_sst.para_st.cap_en);
    ESP_LOGI(TAG, "device id: %d", this_sst.para_st.devId_en);
    ESP_LOGI(TAG, "device location %s", this_sst.para_st.devLoc_cha);

    RegisterDeviceCommands_vd();
    
    if(true == exeResult_bol)
    {
        this_sst.state_st = STATE_INITIALIZED;
        result_st = ESP_OK;
    }

    return(result_st);
}

/**--------------------------------------------------------------------------------------
 * @brief     Starts the devices which are setup per parameter
*//*-----------------------------------------------------------------------------------*/
extern esp_err_t devmgr_GenerateDevices_td(void)
{
    bool exeResult_bol = true;
    esp_err_t result_st = ESP_OK;

    if(STATE_INITIALIZED == this_sst.state_st)
    {
        this_sst.state_st = STATE_DEVICES_ACTIVE;

        switch(this_sst.cap_en)
        {
            case CAPABILITY_DEFAULT:
                exeResult_bol &= CHECK_EXE(GenerateDefaultDevice_st());
                exeResult_bol &= CHECK_EXE(gendev_Activate_st());
                exeResult_bol &= CHECK_EXE(GenerateMijaDevice_st());
                exeResult_bol &= CHECK_EXE(mijasens_Activate_st());
                break;
            case CAPABILITY_DHT22:
                break;
            case CAPABILITY_MIJA_SENS:
                break;
            default:
                break;
        }
    }
    else
    {
        exeResult_bol = false;
    }

    if(false == exeResult_bol)
    {
        result_st = ESP_FAIL;
    }

    return(result_st);
}

/****************************************************************************************/
/* Local functions: */
/**--------------------------------------------------------------------------------------
 * @brief     generate default capability mqtt devices
 * @author    S. Wink
 * @date      24. Jun. 2019
 * @return    true if empty, else false
*//*-----------------------------------------------------------------------------------*/
static esp_err_t GenerateDefaultDevice_st(void)
{
    bool exeResult_bol = true;
    esp_err_t result_st = ESP_OK;
    gendev_param_t param_st;
    //const char *topic_cchp;
    uint16_t idx_u16 = 0U;
    mqttif_substParam_t subsParam_st;
    bool hasMoreSubscriptions_bol;

    // initialize the device first
    exeResult_bol &= CHECK_EXE(gendev_InitializeParameter_st(&param_st));
    param_st.deviceName_chp = this_sst.devName_chp;
    param_st.id_chp = this_sst.id_chp;
    param_st.publishHandler_fp = mqttdrv_Publish_td;
    exeResult_bol &= CHECK_EXE(gendev_Initialize_st(&param_st));

    ESP_LOGD(TAG, "start subscribing gendev topics");

    // subscribe to all topics of this device
    exeResult_bol &= CHECK_EXE(mqttdrv_InitSubscriptParam_td(&subsParam_st));

    hasMoreSubscriptions_bol = gendev_GetSubscriptionByIndex_bol(idx_u16, &subsParam_st);
    ESP_LOGD(TAG, "topic from gendev: %s", subsParam_st.topic_u8a);
    while(hasMoreSubscriptions_bol && (ESP_FAIL != result_st))
    {
        exeResult_bol &= (NULL != mqttdrv_AllocSubs_xp(&subsParam_st));
        idx_u16++;
        hasMoreSubscriptions_bol = gendev_GetSubscriptionByIndex_bol(idx_u16,
                                                                        &subsParam_st);
    }

    if(false == exeResult_bol)
    {
        result_st = ESP_FAIL;
    }
    return(result_st);
}

/**--------------------------------------------------------------------------------------
 * @brief     generate mija capability mqtt devices
 * @author    S. Wink
 * @date      24. Jun. 2019
 * @return    true if empty, else false
*//*-----------------------------------------------------------------------------------*/
static esp_err_t GenerateMijaDevice_st(void)
{
    bool exeResult_bol = true;
    esp_err_t result_st = ESP_OK;
    mijasens_param_t param_st;
    uint16_t idx_u16 = 0U;
    mqttif_substParam_t subsParam_st;
    bool hasMoreSubscriptions_bol;

    // initialize the device first
    exeResult_bol &= CHECK_EXE(mijasens_InitializeParameter_st(&param_st));
    param_st.deviceName_chp = this_sst.devName_chp;
    param_st.id_chp = this_sst.id_chp;
    param_st.publishHandler_fp = mqttdrv_Publish_td;
    exeResult_bol &= CHECK_EXE(mijasens_Initialize_st(&param_st));

    ESP_LOGD(TAG, "start subscribing gendev topics");

    // subscribe to all topics of this device
    exeResult_bol &= CHECK_EXE(mqttdrv_InitSubscriptParam_td(&subsParam_st));

    hasMoreSubscriptions_bol = mijasens_GetSubscriptionByIndex_bol(idx_u16, &subsParam_st);
    ESP_LOGD(TAG, "topic from mijasens: %s", subsParam_st.topic_u8a);
    while(hasMoreSubscriptions_bol && (ESP_FAIL != result_st))
    {
        exeResult_bol &= (NULL != mqttdrv_AllocSubs_xp(&subsParam_st));
        idx_u16++;
        hasMoreSubscriptions_bol = mijasens_GetSubscriptionByIndex_bol(idx_u16,
                                                                        &subsParam_st);
    }

    if(false == exeResult_bol)
    {
        result_st = ESP_FAIL;
    }
    return(result_st);
}

/**--------------------------------------------------------------------------------------
 * @brief     Handler for console command parameter set
 * @author    S. Wink
 * @date      30. Dec. 2019
 * @param     argc_s32  count of argument list
 * @param     argv      pointer to argument list
 * @return    not equal to zero if error detected
*//*-----------------------------------------------------------------------------------*/
static int32_t CmdHandlerChangeParameter_s32(int32_t argc_s32, char** argv_cpp)
{
    int32_t nerrors_s32 = arg_parse(argc_s32, argv_cpp, (void**) &cmdArgsDevice_sts);

    if(0 != nerrors_s32) 
    {
        arg_print_errors(stderr, cmdArgsDevice_sts.end_stp, argv_cpp[0]);
        return 1;
    }

    this_sst.para_st.cap_en = *cmdArgsDevice_sts.cap_stp->ival;
    this_sst.para_st.devId_en = *cmdArgsDevice_sts.id_stp->ival;
    memset(this_sst.para_st.devLoc_cha, 0, sizeof(this_sst.para_st.devLoc_cha));
    memcpy(this_sst.para_st.devLoc_cha, cmdArgsDevice_sts.location_stp->sval,
            sizeof(this_sst.para_st.devLoc_cha));

    CHECK_EXE(paramif_Write_td(deviceParaHdl_xps, (uint8_t *) &this_sst.para_st));

    return(0);
}

/**-------------------------------------------------------------------------------------
 * @brief     Function to register device command line command
 * @author    S. Wink
 * @date      24. Jan. 2019
*//*-----------------------------------------------------------------------------------*/
static void RegisterDeviceCommands_vd(void)
{
    cmdArgsDevice_sts.cap_stp = arg_int0("c", "capa", "<c>",
                                                "Capability id");
    cmdArgsDevice_sts.cap_stp->ival[0] = 0; // set default value
    cmdArgsDevice_sts.id_stp = arg_int0("d", "dev", "<d>",
                                                "Device id");
    cmdArgsDevice_sts.id_stp->ival[0] = 98; // set default value
    cmdArgsDevice_sts.location_stp = arg_str1(NULL, NULL, "<loc>", "location of device");
    cmdArgsDevice_sts.end_stp = arg_end(2);

    const myConsole_cmd_t paramCmd = 
    {
        .command = "dev",
        .help = "Device setup",
        .hint = NULL,
        .func = &CmdHandlerChangeParameter_s32,
        .argtable = &cmdArgsDevice_sts
    };
    CHECK_EXE(myConsole_CmdRegister_td(&paramCmd));
}

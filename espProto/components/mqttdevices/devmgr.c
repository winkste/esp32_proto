/*****************************************************************************************
* FILENAME :        devmgr.c
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
#include "devmgr.h"

#include "stdint.h"
#include "string.h"
#include "esp_err.h"
#include "esp_log.h"
#include "stdbool.h"

#include "utils.h"
#include "mqttif.h"
#include "mqttdrv.h"

#include "gendev.h"

/****************************************************************************************/
/* Local constant defines */

/****************************************************************************************/
/* Local function like makros */

/****************************************************************************************/
/* Local type definitions (enum, struct, union) */

typedef enum moduleState_tag
{
     STATE_NOT_INITIALIZED,
     STATE_INITIALIZED,
     STATE_DEVICES_ACTIVE
}moduleState_t;

typedef enum capType_tag
{
    CAPABILITY_DEFAULT = 0,
    CAPABILITY_DHT22,
    CAPABILITY_UNDEFINED = 0xFF
}capType_t;

typedef struct moduleData_tag
{
     capType_t cap_en;
     moduleState_t state_st;
     char *devName_chp;
     char *id_chp;
}moduleData_t;

/****************************************************************************************/
/* Local functions prototypes: */

/****************************************************************************************/
/* Local variables: */
moduleData_t obj_st =
{
    .cap_en = CAPABILITY_DEFAULT,
    .state_st = STATE_NOT_INITIALIZED,
    .devName_chp = "dev98",
    .id_chp = "chan1"
};

static const char *TAG = "devmgr";

/****************************************************************************************/
/* Global functions (unlimited visibility) */

/**---------------------------------------------------------------------------------------
 * @brief     Initializes the initialization structure of the device manager module
*//*-----------------------------------------------------------------------------------*/
extern esp_err_t devmgr_InitializeParameter(devmgr_param_t *param_stp)
{
    esp_err_t result_st = ESP_OK;

    return(result_st);
}

/**---------------------------------------------------------------------------------------
 * @brief     Initialization of the device manager module
*//*-----------------------------------------------------------------------------------*/
extern esp_err_t devmgr_Initialize(devmgr_param_t *param_stp)
{
    esp_err_t result_st = ESP_OK;

    obj_st.state_st = STATE_INITIALIZED;

    return(result_st);
}

/**---------------------------------------------------------------------------------------
 * @brief     Starts the devices which are setup per parameter
*//*-----------------------------------------------------------------------------------*/
extern void devmgr_GenerateDevices(void)
{
    esp_err_t result_st = ESP_FAIL;

    gendev_param_t iniparam_st;

    if(STATE_INITIALIZED == obj_st.state_st)
    {
        obj_st.state_st = STATE_DEVICES_ACTIVE;

        switch(obj_st.cap_en)
        {
            case CAPABILITY_DEFAULT:
                result_st = gendev_InitializeParameter_st(&iniparam_st);
                iniparam_st.deviceName_chp = obj_st.devName_chp;
                iniparam_st.id_chp = obj_st.id_chp;
                iniparam_st.publishHandler_fp = mqttdrv_Publish_st;
                result_st = gendev_Initialize_st(&iniparam_st);

                mqttdrv_substParam_t subsParam_st;
                result_st = mqttdrv_InitSubParam(&subsParam_st);

                subsParam_st.conn_fp = gendev_GetConnectHandler_fp();
                subsParam_st.discon_fp = gendev_GetDisconnectHandler_fp();
                subsParam_st.dataRecv_fp = gendev_GetDataReceivedHandler_fp();
                subsParam_st.qos_u32 = 0;

                const char *topic_cchp;
                uint16_t idx_u16 = 0;
                topic_cchp = gendev_GetSubsTopics_cchp(idx_u16);
                mqttdrv_subsHdl_t handle_xp;
                while(NULL != topic_cchp)
                {
                    memset(&subsParam_st.topic_u8a[0], 0U, sizeof(subsParam_st.topic_u8a));
                    memcpy(&subsParam_st.topic_u8a[0], topic_cchp,
                            utils_MIN(strlen(topic_cchp), mqttif_MAX_SIZE_OF_TOPIC));
                    handle_xp = mqttdrv_AllocSub_xp(&subsParam_st);
                    result_st = mqttdrv_Subscribe_xp(handle_xp);

                    ESP_LOGI(TAG, "plot all subscriptions");
                    ESP_LOGI(TAG, "actual number of subscriptions in list: %d",
                                mqttdrv_GetNumberOfSubscriptions());

                    idx_u16++;
                    topic_cchp = gendev_GetSubsTopics_cchp(idx_u16);
                }
                break;
            case CAPABILITY_DHT22:
                break;
            default:
                break;
        }
    }
    else
    {

    }
}

/****************************************************************************************/
/* Local functions: */



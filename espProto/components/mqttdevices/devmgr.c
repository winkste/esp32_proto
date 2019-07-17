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
    CAPABILITY_UNDEFINED = 0xFF
}capType_t;

typedef struct objectData_tag
{
     capType_t cap_en;
     objectState_t state_st;
     char *devName_chp;
     char *id_chp;
}objectData_t;

/****************************************************************************************/
/* Local functions prototypes: */
static esp_err_t GenerateDefaultDevice_st(void);

/****************************************************************************************/
/* Local variables: */
static objectData_t obj_sts =
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

    obj_sts.state_st = STATE_INITIALIZED;

    return(result_st);
}

/**---------------------------------------------------------------------------------------
 * @brief     Starts the devices which are setup per parameter
*//*-----------------------------------------------------------------------------------*/
extern void devmgr_GenerateDevices(void)
{
    esp_err_t result_st = ESP_OK;


    if(STATE_INITIALIZED == obj_sts.state_st)
    {
        obj_sts.state_st = STATE_DEVICES_ACTIVE;

        switch(obj_sts.cap_en)
        {
            case CAPABILITY_DEFAULT:
                if(ESP_OK == GenerateDefaultDevice_st())
                {
                    if(ESP_OK == gendev_Activate_st())
                    {
                        ESP_LOGI(TAG, "generic device generated and activated...");
                    }
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

    if(ESP_FAIL == result_st)
    {
        ESP_LOGW(TAG, "unexpected error during device generation");
    }
}

/****************************************************************************************/
/* Local functions: */
/**---------------------------------------------------------------------------------------
 * @brief     generate default capability mqtt devices
 * @author    S. Wink
 * @date      24. Jun. 2019
 * @return    true if empty, else false
*//*------------------------------------------------------------------------------------*/
static esp_err_t GenerateDefaultDevice_st(void)
{
    esp_err_t result_st = ESP_OK;
    gendev_param_t iniparam_st;
    //const char *topic_cchp;
    uint16_t idx_u16 = 0;
    mqttif_substParam_t subsParam_st;

    // initialize the device first
    result_st = gendev_InitializeParameter_st(&iniparam_st);
    iniparam_st.deviceName_chp = obj_sts.devName_chp;
    iniparam_st.id_chp = obj_sts.id_chp;
    iniparam_st.publishHandler_fp = mqttdrv_Publish_st;
    result_st = gendev_Initialize_st(&iniparam_st);

    ESP_LOGD(TAG, "start subscribing gendev topics");

    // subscribe to all topics of this device
    result_st = mqttdrv_InitSubscriptParam(&subsParam_st);

    bool hasMoreSubscriptions_bol;
    hasMoreSubscriptions_bol = gendev_GetSubscriptionByIndex_bol(idx_u16, &subsParam_st);
    ESP_LOGD(TAG, "topic from gendev: %s", subsParam_st.topic_u8a);
    while(hasMoreSubscriptions_bol && (ESP_FAIL != result_st))
    {
        if(NULL == mqttdrv_AllocSub_xp(&subsParam_st))
        {
            result_st = ESP_FAIL;
        }
        idx_u16++;
        hasMoreSubscriptions_bol = gendev_GetSubscriptionByIndex_bol(idx_u16,
                                                                        &subsParam_st);
    }

    /*subsParam_st.conn_fp = gendev_GetConnectHandler_fp();
    subsParam_st.discon_fp = gendev_GetDisconnectHandler_fp();
    subsParam_st.dataRecv_fp = gendev_GetDataReceivedHandler_fp();
    subsParam_st.qos_u32 = 0;

    topic_cchp = gendev_GetSubsTopics_cchp(idx_u16);
    while((NULL != topic_cchp) && (ESP_FAIL != result_st))
    {
        memset(&subsParam_st.topic_u8a[0], 0U, sizeof(subsParam_st.topic_u8a));
        memcpy(&subsParam_st.topic_u8a[0], topic_cchp,
                utils_MIN(strlen(topic_cchp), mqttif_MAX_SIZE_OF_TOPIC));
        if(NULL == mqttdrv_AllocSub_xp(&subsParam_st))
        {
            result_st = ESP_FAIL;
        }
        idx_u16++;
        topic_cchp = gendev_GetSubsTopics_cchp(idx_u16);
    }*/

    return(result_st);
}

/*****************************************************************************************
* FILENAME :        gendev.c
*
* DESCRIPTION :
*       This module
*
* AUTHOR :    Stephan Wink        CREATED ON :    07.04.2019
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
#include "gendev.h"

#include "stdbool.h"
#include "string.h"
#include "esp_log.h"
#include "esp_err.h"
#include "mqttif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/timers.h"

#include "myVersion.h"

/****************************************************************************************/
/* Local constant defines */

#define MQTT_PUB_FW_IDENT         "/s/gen/fwident" //firmware identification
#define MQTT_PUB_FW_VERSION       "/s/gen/fwversion" //firmware version
#define MQTT_PUB_FW_DESC          "/s/gen/desc" //firmware description
#define MQTT_PUB_DEV_ROOM         "/s/gen/room" //firmware room
#define MQTT_PUB_CAP              "/s/gen/cap"  // send capability
#define MQTT_PUB_TRACE            "/s/gen/trac" // send trace channel
#define MQTT_SUB_COMMAND          "/r/gen/cmd" // command message for generic read commands
#define MQTT_SUB_BCAST            "bcast/r/gen/cmd" // broadcast command message
#define MQTT_CLIENT               MQTT_DEFAULT_DEVICE // just a name used to talk to MQTT broker
#define MQTT_PAYLOAD_CMD_INFO     "INFO"
#define MQTT_SUBSCRIPTIONS_NUM    2U
#define MAX_PUB_WAIT              1000


#ifndef BIT0
    #define BIT0    0x00000000
    #define BIT1    0x00000001
    #define BIT2    0x00000002
    #define BIT3    0x00000004
    #define BIT4    0x00000008
#endif

/****************************************************************************************/
/* Local function like makros */

/****************************************************************************************/
/* Local type definitions (enum, struct, union) */
typedef enum objState_tag
{
     STATE_NOT_INITIALIZED,
     STATE_INITIALIZED,
     STATE_CONNECTED,
     STATE_DISCONNECTED
}objState_t;

typedef struct objdata_tag
{
    gendev_param_t param_st;
    objState_t state_en;
}objdata_t;

/****************************************************************************************/
/* Local functions prototypes: */
static void OnConnectionHandler_vd(void);
static void OnDisconnectionHandler_vd(void);
static esp_err_t OnDataReceivedHandler_st(mqttif_msg_t *msg_stp);

static void TimerCallback_vd(TimerHandle_t xTimer);
static void Task_vd(void *pvParameters);

/****************************************************************************************/
/* Local variables: */

static objdata_t singleton_sts;

static const char *subscriptions_cchsap[MQTT_SUBSCRIPTIONS_NUM] =
{
    MQTT_SUB_COMMAND, // command message for generic read commands
    MQTT_SUB_BCAST // write message for geeneric broadcast
};

static const int MQTT_CON       = BIT0;
static const int MQTT_DISCON    = BIT1;
static const int CYCLE_TIMER    = BIT2;
static const int MOD_ERROR      = BIT3;
static const int MQTT_DATA      = BIT4;

static EventGroupHandle_t gendevEventGroup_sts;

const char *TAG = "gendev";

static TimerHandle_t cycleTimer_sts;

/****************************************************************************************/
/* Global functions (unlimited visibility) */

/**---------------------------------------------------------------------------------------
 * @brief     Initializes the initialization structure of the generic device module
*//*-----------------------------------------------------------------------------------*/
esp_err_t gendev_InitializeParameter_st(gendev_param_t *param_stp)
{
    esp_err_t result_st = ESP_FAIL;

    singleton_sts.state_en = STATE_NOT_INITIALIZED;
    singleton_sts.param_st.deviceName_cchp = "dev99";
    singleton_sts.param_st.id_u8c = 0U;
    singleton_sts.param_st.publishHandler_fp = NULL;

    if(NULL != param_stp)
    {
        param_stp->publishHandler_fp = NULL;
        param_stp->deviceName_cchp = "dev99";
        param_stp->id_u8c = 0U;

        result_st = ESP_OK;
    }

    return(result_st);
}

/**---------------------------------------------------------------------------------------
 * @brief     Initialization of the generic device module
*//*-----------------------------------------------------------------------------------*/
esp_err_t gendev_Initialize_st(gendev_param_t *param_stp)
{
    esp_err_t result_st = ESP_FAIL;

    if(NULL != param_stp)
    {
        singleton_sts.param_st.deviceName_cchp = param_stp->deviceName_cchp;
        singleton_sts.param_st.id_u8c = param_stp->id_u8c;
        singleton_sts.param_st.publishHandler_fp = param_stp->publishHandler_fp;

        singleton_sts.state_en = STATE_INITIALIZED;

        result_st = ESP_OK;
    }

    cycleTimer_sts = xTimerCreate("Timer", pdMS_TO_TICKS(10000), true, (void *) 0,
                                     TimerCallback_vd);

    /* start the mqtt task */
    xTaskCreate(Task_vd, "gendevTask", 4096, NULL, 5, NULL);

    return(result_st);
}

/**---------------------------------------------------------------------------------------
 * @brief     Get subscribe topics from the generic device
*//*-----------------------------------------------------------------------------------*/
const char* gendev_GetSubsTopics_cchp(uint16_t idx_u16)
{
    uint16_t elements_u16 = sizeof(subscriptions_cchsap) / sizeof(subscriptions_cchsap[0]);

    if(elements_u16 > idx_u16)
    {
        return(subscriptions_cchsap[idx_u16]);
    }
    else
    {
        return(NULL);
    }
}

/**---------------------------------------------------------------------------------------
 * @brief     Get the mqtt connection success function handler
*//*-----------------------------------------------------------------------------------*/
mqttif_Connected_td gendev_GetConnectHandler_fp(void)
{
    return(OnConnectionHandler_vd);
}

/**---------------------------------------------------------------------------------------
 * @brief     Get the mqtt disconnection success function handler
*//*-----------------------------------------------------------------------------------*/
mqttif_Disconnected_td gendev_GetDisconnectHandler_fp(void)
{
    return(OnDisconnectionHandler_vd);
}

/**---------------------------------------------------------------------------------------
 * @brief     Get the mqtt data received function handler
*//*-----------------------------------------------------------------------------------*/
mqttif_DataReceived_td gendev_GetDataReceivedHandler_fp(void)
{
    return(OnDataReceivedHandler_st);
}

/**---------------------------------------------------------------------------------------
 * @brief     Activate the generic device function
*//*-----------------------------------------------------------------------------------*/
esp_err_t gendev_Activate_st(void)
{
    return(ESP_FAIL);
}

/**---------------------------------------------------------------------------------------
 * @brief     Deactivate the generic device function
*//*-----------------------------------------------------------------------------------*/
esp_err_t gendev_Deactivate_st(void)
{
    return(ESP_FAIL);
}

/****************************************************************************************/
/* Local functions: */

/**---------------------------------------------------------------------------------------
 * @brief     Handler when connected to mqtt broker
 * @author    S. Wink
 * @date      08. Apr. 2019
 * @return    n/a
*//*-----------------------------------------------------------------------------------*/
static void OnConnectionHandler_vd(void)
{
    xEventGroupSetBits(gendevEventGroup_sts, MQTT_CON);
}

/**---------------------------------------------------------------------------------------
 * @brief     Handler when disconnected from mqtt broker
 * @author    S. Wink
 * @date      08. Apr. 2019
 * @return    n/a
*//*-----------------------------------------------------------------------------------*/
static void OnDisconnectionHandler_vd(void)
{
    xEventGroupSetBits(gendevEventGroup_sts, MQTT_DISCON);
}

/**---------------------------------------------------------------------------------------
 * @brief     Handler when disconnected from mqtt broker
 * @author    S. Wink
 * @date      08. Apr. 2019
 * @return    n/a
*//*-----------------------------------------------------------------------------------*/
static esp_err_t OnDataReceivedHandler_st(mqttif_msg_t *msg_stp)
{
    esp_err_t result_st = ESP_OK;
    mqttif_msg_t msg_st;
    char msgData_ca[50];

    ESP_LOGI(TAG, "message topic:%s received with data:%s",
                    msg_stp->topic_chp, msg_stp->data_chp);
    xEventGroupSetBits(gendevEventGroup_sts, MQTT_DATA);

    if(0U == strcmp(msg_stp->topic_chp, MQTT_SUB_COMMAND))
    {
        if(0U == strcmp(msg_stp->data_chp, MQTT_PAYLOAD_CMD_INFO))
        {
            msg_st.data_chp = &msgData_ca[0];

            if(ESP_OK == result_st)
            {
                // send the firmware identifier
                msg_st.topic_chp = MQTT_PUB_FW_IDENT;
                msg_st.dataLen_u32 = sprintf(msg_st.data_chp, "Firmware PN: %s",
                                                myVersion_GetFwIdentifier_cch());
                result_st = singleton_sts.param_st.publishHandler_fp(&msg_st,
                                                                        MAX_PUB_WAIT);
            }
            if(ESP_OK == result_st)
            {
                // send the firmware version
                msg_st.topic_chp = MQTT_PUB_FW_VERSION;
                msg_st.dataLen_u32 = sprintf(msg_st.data_chp, "Firmware Version: %s",
                                                myVersion_GetFwVersion_cch());
                result_st = singleton_sts.param_st.publishHandler_fp(&msg_st,
                                                                        MAX_PUB_WAIT);
            }

            if(ESP_OK == result_st)
            {
                // send the firmware version
                msg_st.topic_chp = MQTT_PUB_FW_DESC;
                msg_st.dataLen_u32 = sprintf(msg_st.data_chp, "Firmware Description: %s",
                                                myVersion_GetFwDescription_cch());
                result_st = singleton_sts.param_st.publishHandler_fp(&msg_st, MAX_PUB_WAIT);
            }
        }
        else
        {
            ESP_LOGW(TAG, "unexpected data %s", msg_stp->data_chp);
            result_st = ESP_FAIL;
        }
    }
    else
    {
        ESP_LOGW(TAG, "unexpected topic %s", msg_stp->topic_chp);
        result_st = ESP_FAIL;
    }

    xEventGroupSetBits(gendevEventGroup_sts, MQTT_DATA);
    return(result_st);
}

/**---------------------------------------------------------------------------------------
 * @brief     callback function for the timer event handler
 * @author    S. Wink
 * @date      24. Jan. 2019
 * @param     xTimer      handle to timer
*//*-----------------------------------------------------------------------------------*/
static void TimerCallback_vd(TimerHandle_t xTimer)
{
    xEventGroupSetBits(gendevEventGroup_sts, CYCLE_TIMER);
}

/**---------------------------------------------------------------------------------------
 * @brief     task routine for the mqtt handling
 * @author    S. Wink
 * @date      24. Jan. 2019
 * @param     pvParameters      interface variable from freertos
*//*-----------------------------------------------------------------------------------*/
static void Task_vd(void *pvParameters)
{
    EventBits_t uxBits_st;
    uint32_t bits_u32 = MQTT_CON | MQTT_DISCON | CYCLE_TIMER | MOD_ERROR | MQTT_DATA;

    ESP_LOGI(TAG, "mqttTask started...");
    while(1)
    {
        uxBits_st = xEventGroupWaitBits(gendevEventGroup_sts, bits_u32,
                                         true, false, portMAX_DELAY); // @suppress("Symbol is not resolved")

        if(0 != (uxBits_st & MQTT_CON))
        {
            ESP_LOGI(TAG, "mqtt connected");
        }
        if(0 != (uxBits_st & MQTT_DISCON))
        {
            ESP_LOGI(TAG, "mqtt disconnected");

        }
        if(0 != (uxBits_st & CYCLE_TIMER))
        {
            ESP_LOGI(TAG, "cycle timer expired");

        }
        if(0 != (uxBits_st & MOD_ERROR))
        {
            ESP_LOGI(TAG, "module error");
        }
        if(0 != (uxBits_st & MQTT_DATA))
        {
            ESP_LOGI(TAG, "mqtt data received");
        }
    }
}


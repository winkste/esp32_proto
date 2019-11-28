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
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/timers.h"

#include "mqttif.h"

//#include "../../main/appIdent.h"
#include "appIdent.h"
#include "utils.h"

/****************************************************************************************/
/* Local constant defines */

#define MQTT_PUB_FW_IDENT         "gen/fwident" //firmware identification
#define MQTT_PUB_FW_VERSION       "gen/fwversion" //firmware version
#define MQTT_PUB_FW_DESC          "gen/desc" //firmware description
#define MQTT_PUB_HEALTH           "gen/health" //health counter of application

#define MQTT_PUB_DEV_ROOM         "gen/room" //firmware room
#define MQTT_PUB_CAP              "gen/cap"  // send capability
#define MQTT_PUB_TRACE            "gen/trac" // send trace channel
#define MQTT_SUB_COMMAND          "gen/cmd" // command message for generic read commands
#define MQTT_CLIENT               MQTT_DEFAULT_DEVICE // just a name used to talk to MQTT broker
#define MQTT_PAYLOAD_CMD_INFO     "INFO"
#define MQTT_SUBSCRIPTIONS_NUM    2U
#define MAX_PUB_WAIT              10000


#ifndef BIT0
    #define BIT0    0x00000000
    #define BIT1    0x00000001
    #define BIT2    0x00000002
    #define BIT3    0x00000004
    #define BIT4    0x00000008
    #define BIT5    0x00000010
#endif

/****************************************************************************************/
/* Local function like makros */

/****************************************************************************************/
/* Local type definitions (enum, struct, union) */
typedef enum objectState_tag
{
     STATE_NOT_INITIALIZED,
     STATE_INITIALIZED,
     STATE_ACTIVE,
     STATE_STOPPED,
     STATE_CONNECTED,
     STATE_DISCONNECTED
}objectState_t;

typedef struct moduleData_tag
{
    gendev_param_t param_st;
    objectState_t state_en;
    mqttif_msg_t pubMsg_st;
    uint32_t healthCounter_u32;
    EventGroupHandle_t eventGroup_st;
    TimerHandle_t cycleTimer_st;
    char subs_chap[MQTT_SUBSCRIPTIONS_NUM][mqttif_MAX_SIZE_OF_TOPIC];
    uint16_t subsCounter_u16;
}objectData_t;

/****************************************************************************************/
/* Local functions prototypes: */
static void OnConnectionHandler_vd(void);
static void OnDisconnectionHandler_vd(void);
static void SendInfoRecord_vd(void);
static esp_err_t OnDataReceivedHandler_st(mqttif_msg_t *msg_stp);

static void SendHealthCounter_vd(void);
static void TimerCallback_vd(TimerHandle_t xTimer);
static void Task_vd(void *pvParameters);

/****************************************************************************************/
/* Local variables: */

static objectData_t singleton_sts;

static const char *subscriptions_cchsap[MQTT_SUBSCRIPTIONS_NUM] =
{
    MQTT_SUB_COMMAND, // command message for generic read commands
    MQTT_SUB_COMMAND  // write message for geeneric broadcast
};

static const int MQTT_CONNECT       = BIT0;
static const int MQTT_DISCONNECT    = BIT1;
static const int CYCLE_TIMER        = BIT2;
static const int MOD_ERROR          = BIT3;
static const int MQTT_INFO_REQUEST  = BIT4;

static const char *TAG = "gendev";

/****************************************************************************************/
/* Global functions (unlimited visibility) */

/**---------------------------------------------------------------------------------------
 * @brief     Initializes the initialization structure of the generic device module
*//*-----------------------------------------------------------------------------------*/
esp_err_t gendev_InitializeParameter_st(gendev_param_t *param_stp)
{
    esp_err_t result_st = ESP_FAIL;

    singleton_sts.state_en = STATE_NOT_INITIALIZED;
    singleton_sts.param_st.deviceName_chp = "dev99";
    singleton_sts.param_st.id_chp = "chan0";
    singleton_sts.param_st.publishHandler_fp = NULL;
    singleton_sts.healthCounter_u32 = 0L;

    if(NULL != param_stp)
    {
        param_stp->publishHandler_fp = NULL;
        param_stp->deviceName_chp = "dev99";
        param_stp->id_chp = "chan0";

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


    esp_log_level_set(TAG, ESP_LOG_DEBUG);

    ESP_LOGD(TAG, "initialization started...");
    if(NULL != param_stp)
    {
        /* copy parameters and initialize internal module data */
        singleton_sts.param_st.deviceName_chp = param_stp->deviceName_chp;
        singleton_sts.param_st.id_chp = param_stp->id_chp;
        singleton_sts.param_st.publishHandler_fp = param_stp->publishHandler_fp;
        singleton_sts.pubMsg_st.dataLen_u32 = 0;
        singleton_sts.pubMsg_st.topicLen_u32 = 0;
        singleton_sts.pubMsg_st.topic_chp =
                                        malloc(mqttif_MAX_SIZE_OF_TOPIC * sizeof(char));
        singleton_sts.pubMsg_st.data_chp =
                                        malloc(mqttif_MAX_SIZE_OF_DATA * sizeof(char));
        singleton_sts.pubMsg_st.qos_s32 = 1;
        singleton_sts.pubMsg_st.retain_s32 = 0;
        singleton_sts.subsCounter_u16 = 0U;
        memset(&singleton_sts.subs_chap[0][0], 0U, sizeof(singleton_sts.subs_chap));

        /* create subscription topics */
        utils_BuildReceiveTopic_chp(singleton_sts.param_st.deviceName_chp,
                                                singleton_sts.param_st.id_chp,
                                                subscriptions_cchsap[0],
                                                &singleton_sts.subs_chap[0][0]);
        singleton_sts.subsCounter_u16++;

        utils_BuildReceiveTopicBCast_chp(subscriptions_cchsap[1],
                                                &singleton_sts.subs_chap[1][0]);
        singleton_sts.subsCounter_u16++;

        singleton_sts.state_en = STATE_INITIALIZED;

        result_st = ESP_OK;
    }

    singleton_sts.eventGroup_st = xEventGroupCreate();
    if(NULL == singleton_sts.eventGroup_st)
    {
        ESP_LOGE(TAG, "eventGroup creation error...");
        result_st = ESP_FAIL;
    }

    singleton_sts.cycleTimer_st = xTimerCreate("Timer", pdMS_TO_TICKS(10000), true,
                                                (void *) 0, TimerCallback_vd);
    if(NULL == singleton_sts.cycleTimer_st)
    {
        ESP_LOGE(TAG, "timer creation error...");
        result_st = ESP_FAIL;
    }

    /* start the mqtt task */
    if(!xTaskCreate(Task_vd, "gendevTask", 4096, NULL, 5, NULL))
    {
        ESP_LOGE(TAG, "task creation error...");
        result_st = ESP_FAIL;
    }

    ESP_LOGD(TAG, "initialization completed...");
    return(result_st);
}

/**---------------------------------------------------------------------------------------
 * @brief     Get subscribe topics from the device
*//*-----------------------------------------------------------------------------------*/
const char* gendev_GetSubsTopics_cchp(uint16_t idx_u16)
{
    if(singleton_sts.subsCounter_u16 > idx_u16)
    {
        return(&singleton_sts.subs_chap[idx_u16][0]);
    }
    else
    {
        return(NULL);
    }
}

/**---------------------------------------------------------------------------------------
 * @brief     Get subscriptions from the generic device by index
 * @author    S. Wink
 * @date      25. Mar. 2019
 * @param     idx_u16    index of topic, can be used as iterator
 * @param     dest_stp   pointer to the subscription parameter structure
 * @return    returns false if the index was out of bounce and no subscription was
 *              copied
*//*-----------------------------------------------------------------------------------*/
extern bool gendev_GetSubscriptionByIndex_bol(uint16_t idx_u16,
                                                mqttif_substParam_t *dest_stp)
{
    bool indexIsValid_bol = false;

    if(singleton_sts.subsCounter_u16 > idx_u16)
    {
        indexIsValid_bol = true;
        dest_stp->conn_fp = OnConnectionHandler_vd;
        dest_stp->discon_fp = OnDisconnectionHandler_vd;
        dest_stp->dataRecv_fp = OnDataReceivedHandler_st;
        dest_stp->qos_u32 = 0;

        memset(&dest_stp->topic_u8a[0], 0U, sizeof(dest_stp->topic_u8a));
        memcpy(&dest_stp->topic_u8a[0], &singleton_sts.subs_chap[idx_u16][0],
             utils_MIN(strlen(singleton_sts.subs_chap[idx_u16]),
                         mqttif_MAX_SIZE_OF_TOPIC));
    }
    else
    {
        indexIsValid_bol = false;
    }

    return(indexIsValid_bol);
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
    esp_err_t result_st = ESP_FAIL;

    if(STATE_INITIALIZED == singleton_sts.state_en)
    {
        if(NULL != singleton_sts.cycleTimer_st)
        {
            if(pdPASS == xTimerStart(singleton_sts.cycleTimer_st, 0))
            {
                result_st = ESP_OK;
                singleton_sts.state_en = STATE_ACTIVE;
            }
        }
    }

    if(ESP_FAIL == result_st)
    {
        ESP_LOGE(TAG, "error during activation detected...");
    }

    return(result_st);
}

/**---------------------------------------------------------------------------------------
 * @brief     Stopps the generic device function
*//*-----------------------------------------------------------------------------------*/
esp_err_t gendev_Stopp_st(void)
{
    esp_err_t result_st = ESP_FAIL;

    if(STATE_NOT_INITIALIZED != singleton_sts.state_en)
    {
        if(NULL != singleton_sts.cycleTimer_st)
        {
            if(pdPASS == xTimerStop(singleton_sts.cycleTimer_st, 0))
            {
                result_st = ESP_OK;
                singleton_sts.state_en = STATE_STOPPED;
            }
        }
    }

    if(ESP_FAIL == result_st)
    {
        ESP_LOGE(TAG, "error during stopping detected...");
    }

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
    xEventGroupSetBits(singleton_sts.eventGroup_st, MQTT_CONNECT);
}

/**---------------------------------------------------------------------------------------
 * @brief     Handler when disconnected from mqtt broker
 * @author    S. Wink
 * @date      08. Apr. 2019
 * @return    n/a
*//*-----------------------------------------------------------------------------------*/
static void OnDisconnectionHandler_vd(void)
{
    xEventGroupSetBits(singleton_sts.eventGroup_st, MQTT_DISCONNECT);
}

/**--------------------------------------------------------------------------------------
 * @brief     function to send firmware info record
 * @author    S. Wink
 * @date      29. May. 2019
 * @return    n/a
*//*-----------------------------------------------------------------------------------*/
static void SendInfoRecord_vd(void)
{
    esp_err_t result_st = ESP_OK;


    ESP_LOGD(TAG, "send firmware identifier...");
    utils_BuildSendTopic_chp(singleton_sts.param_st.deviceName_chp,
                                singleton_sts.param_st.id_chp,
                                MQTT_PUB_FW_IDENT,
                                singleton_sts.pubMsg_st.topic_chp);

    singleton_sts.pubMsg_st.topicLen_u32 =
            strlen(singleton_sts.pubMsg_st.topic_chp);
    singleton_sts.pubMsg_st.dataLen_u32 =
            sprintf(singleton_sts.pubMsg_st.data_chp, "Firmware PN: %s",
                                 appIdent_GetFwIdentifier_cch());
    ESP_LOGD(TAG, "topic:%s, data:%s", singleton_sts.pubMsg_st.topic_chp,
                                        singleton_sts.pubMsg_st.data_chp);
    result_st = singleton_sts.param_st.publishHandler_fp(&singleton_sts.pubMsg_st,
                                                            MAX_PUB_WAIT);
    ESP_LOGD(TAG, "send firmware version...");
    if(ESP_OK == result_st)
    {
        utils_BuildSendTopic_chp(singleton_sts.param_st.deviceName_chp,
                                 singleton_sts.param_st.id_chp,
                                 MQTT_PUB_FW_VERSION,
                                 singleton_sts.pubMsg_st.topic_chp);
        singleton_sts.pubMsg_st.topicLen_u32 = strlen(singleton_sts.pubMsg_st.topic_chp);
        singleton_sts.pubMsg_st.dataLen_u32 =
                sprintf(singleton_sts.pubMsg_st.data_chp, "Firmware Version: %s",
                                 appIdent_GetFwVersion_cch());
        ESP_LOGD(TAG, "topic:%s, data:%s", singleton_sts.pubMsg_st.topic_chp,
                                            singleton_sts.pubMsg_st.data_chp);
        result_st = singleton_sts.param_st.publishHandler_fp(&singleton_sts.pubMsg_st,
                                                                MAX_PUB_WAIT);
    }

    ESP_LOGD(TAG, "send firmware description...");
    if(ESP_OK == result_st)
    {
        utils_BuildSendTopic_chp(singleton_sts.param_st.deviceName_chp,
                                         singleton_sts.param_st.id_chp,
                                         MQTT_PUB_FW_DESC,
                                         singleton_sts.pubMsg_st.topic_chp);
        singleton_sts.pubMsg_st.topicLen_u32 = strlen(singleton_sts.pubMsg_st.topic_chp);
        singleton_sts.pubMsg_st.dataLen_u32 =
                sprintf(singleton_sts.pubMsg_st.data_chp, "Firmware Description: %s",
                                        appIdent_GetFwDescription_cch());
        ESP_LOGD(TAG, "topic:%s, data:%s", singleton_sts.pubMsg_st.topic_chp,
                                            singleton_sts.pubMsg_st.data_chp);
        result_st = singleton_sts.param_st.publishHandler_fp(&singleton_sts.pubMsg_st,
                                                                MAX_PUB_WAIT);
    }
}

/**--------------------------------------------------------------------------------------
 * @brief     Handler when disconnected from mqtt broker
 * @author    S. Wink
 * @date      08. Apr. 2019
 * @return    n/a
*//*-----------------------------------------------------------------------------------*/
static esp_err_t OnDataReceivedHandler_st(mqttif_msg_t *msg_stp)
{
    esp_err_t result_st = ESP_OK;

    ESP_LOGD(TAG, "message topic:%.*s received with data:%.*s",
            msg_stp->topicLen_u32, msg_stp->topic_chp,
            msg_stp->dataLen_u32, msg_stp->data_chp);

    if(   (0U == strncmp(msg_stp->topic_chp, &singleton_sts.subs_chap[0][0],
                        msg_stp->topicLen_u32))
       || (0U == strncmp(msg_stp->topic_chp, &singleton_sts.subs_chap[1][0],
               msg_stp->topicLen_u32)))
    {
        if(   (0U != msg_stp->dataLen_u32)
           && (0U == strncmp(msg_stp->data_chp, MQTT_PAYLOAD_CMD_INFO,
                   msg_stp->dataLen_u32)))
        {
            // generic info command received, notify task for publishing
            // information record
            xEventGroupSetBits(singleton_sts.eventGroup_st, MQTT_INFO_REQUEST);
        }
        else
        {
            ESP_LOGW(TAG, "unexpected data received: %.*s", msg_stp->dataLen_u32,
                                                    msg_stp->data_chp);
            result_st = ESP_FAIL;
        }
    }
    else
    {
        ESP_LOGW(TAG, "unexpected topic received: %.*s", msg_stp->topicLen_u32,
                                                msg_stp->topic_chp);
        result_st = ESP_FAIL;
    }

    return(result_st);
}

/**---------------------------------------------------------------------------------------
 * @brief     function to send health counter
 * @author    S. Wink
 * @date      29. May. 2019
 * @return    n/a
*//*-----------------------------------------------------------------------------------*/
static void SendHealthCounter_vd(void)
{
    esp_err_t result_st = ESP_FAIL;

    utils_BuildSendTopic_chp(singleton_sts.param_st.deviceName_chp,
                                     singleton_sts.param_st.id_chp,
                                     MQTT_PUB_HEALTH,
                                     singleton_sts.pubMsg_st.topic_chp);
    singleton_sts.pubMsg_st.topicLen_u32 = strlen(singleton_sts.pubMsg_st.topic_chp);
    singleton_sts.pubMsg_st.dataLen_u32 =
         sprintf(singleton_sts.pubMsg_st.data_chp, "%d",singleton_sts.healthCounter_u32);
    result_st = singleton_sts.param_st.publishHandler_fp(&singleton_sts.pubMsg_st,
                                                            MAX_PUB_WAIT);
    ESP_LOGD(TAG, "publish: %s :: %s", singleton_sts.pubMsg_st.topic_chp,
                                        singleton_sts.pubMsg_st.data_chp);
    if(ESP_FAIL == result_st)
    {
        ESP_LOGE(TAG, "error during publishing of health counter");
    }
}

/**---------------------------------------------------------------------------------------
 * @brief     callback function for the timer event handler
 * @author    S. Wink
 * @date      24. Jan. 2019
 * @param     xTimer      handle to timer
*//*-----------------------------------------------------------------------------------*/
static void TimerCallback_vd(TimerHandle_t xTimer)
{
    ESP_LOGD(TAG, "timer callback...");
    xEventGroupSetBits(singleton_sts.eventGroup_st, CYCLE_TIMER);
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
    uint32_t bits_u32 =   MQTT_CONNECT | MQTT_DISCONNECT | CYCLE_TIMER
                        | MOD_ERROR | MQTT_INFO_REQUEST;

    ESP_LOGD(TAG, "gendev-task started...");
    while(1)
    {
        uxBits_st = xEventGroupWaitBits(singleton_sts.eventGroup_st, bits_u32,
                                         true, false, portMAX_DELAY); // @suppress("Symbol is not resolved")

        if(0 != (uxBits_st & MQTT_CONNECT))
        {
            ESP_LOGD(TAG, "mqtt connected and topic subscribed");
        }
        if(0 != (uxBits_st & MQTT_DISCONNECT))
        {
            ESP_LOGD(TAG, "mqtt disconnected");

        }
        if(0 != (uxBits_st & CYCLE_TIMER))
        {
            ESP_LOGD(TAG, "CYCLE_TIMER event...");
            singleton_sts.healthCounter_u32++;
            SendHealthCounter_vd();
        }
        if(0 != (uxBits_st & MOD_ERROR))
        {
            ESP_LOGE(TAG, "unexpected module error detected...");
        }
        if(0 != (uxBits_st & MQTT_INFO_REQUEST))
        {
            ESP_LOGD(TAG, "mqtt information request received...");
            SendInfoRecord_vd();
        }
    }
}


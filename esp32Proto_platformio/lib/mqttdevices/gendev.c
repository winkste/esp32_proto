/****************************************************************************************
* FILENAME :        gendev.c
*
* SHORT DESCRIPTION:
*   Source file for the gendev module.  
*
* DETAILED DESCRIPTION :   N/A  
*
* AUTHOR :    Stephan Wink        CREATED ON :    07. Apr. 2019
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
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
****************************************************************************************/

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

#define MODULE_TAG                  "gendev"

/****************************************************************************************/
/* Local function like makros */

#define CHECK_EXE(arg) utils_CheckAndLogExecution_bol(MODULE_TAG, arg, __LINE__)

/****************************************************************************************/
/* Local type definitions (enum, struct, union) */
typedef enum objectState_tag
{
     STATE_NOT_INITIALIZED,
     STATE_INITIALIZED,
     STATE_ACTIVE,
     STATE_DEACTIVATED,
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

static objectData_t this_sst;

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

static const char *TAG = MODULE_TAG;

/****************************************************************************************/
/* Global functions (unlimited visibility) */

/**---------------------------------------------------------------------------------------
 * @brief     Initializes the initialization structure of the generic device module
 * @author    S. Wink
 * @date      25. Mar. 2019
*//*-----------------------------------------------------------------------------------*/
esp_err_t gendev_InitializeParameter_st(gendev_param_t *param_stp)
{
    esp_err_t result_st = ESP_FAIL;

    this_sst.state_en = STATE_NOT_INITIALIZED;
    this_sst.param_st.deviceName_chp = "dev99";
    this_sst.param_st.id_chp = "chan0";
    this_sst.param_st.publishHandler_fp = NULL;
    this_sst.healthCounter_u32 = 0L;

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
 * @author    S. Wink
 * @date      25. Mar. 2019
*//*-----------------------------------------------------------------------------------*/
esp_err_t gendev_Initialize_st(gendev_param_t *param_stp)
{
    esp_err_t result_st = ESP_FAIL;


    esp_log_level_set(TAG, ESP_LOG_DEBUG);

    ESP_LOGD(TAG, "initialization started...");
    if(NULL != param_stp)
    {
        /* copy parameters and initialize internal module data */
        this_sst.param_st.deviceName_chp = param_stp->deviceName_chp;
        this_sst.param_st.id_chp = param_stp->id_chp;
        this_sst.param_st.publishHandler_fp = param_stp->publishHandler_fp;
        this_sst.pubMsg_st.dataLen_u32 = 0;
        this_sst.pubMsg_st.topicLen_u32 = 0;
        this_sst.pubMsg_st.topic_chp = malloc(mqttif_MAX_SIZE_OF_TOPIC * sizeof(char));
        this_sst.pubMsg_st.data_chp = malloc(mqttif_MAX_SIZE_OF_DATA * sizeof(char));
        this_sst.pubMsg_st.qos_s32 = 1;
        this_sst.pubMsg_st.retain_s32 = 0;
        this_sst.subsCounter_u16 = 0U;
        memset(&this_sst.subs_chap[0][0], 0U, sizeof(this_sst.subs_chap));

        /* create subscription topics */
        utils_BuildReceiveTopic_chp(this_sst.param_st.deviceName_chp,
                                                this_sst.param_st.id_chp,
                                                subscriptions_cchsap[0],
                                                &this_sst.subs_chap[0][0]);
        this_sst.subsCounter_u16++;

        utils_BuildReceiveTopicBCast_chp(subscriptions_cchsap[1],
                                                &this_sst.subs_chap[1][0]);
        this_sst.subsCounter_u16++;

        this_sst.state_en = STATE_INITIALIZED;

        result_st = ESP_OK;
    }

    this_sst.eventGroup_st = xEventGroupCreate();
    if(NULL == this_sst.eventGroup_st)
    {
        ESP_LOGE(TAG, "eventGroup creation error...");
        result_st = ESP_FAIL;
    }

    this_sst.cycleTimer_st = xTimerCreate("Timer", pdMS_TO_TICKS(10000), true,
                                                (void *) 0, TimerCallback_vd);
    if(NULL == this_sst.cycleTimer_st)
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
 * @brief     Get subscriptions from the generic device by index
 * @author    S. Wink
 * @date      25. Mar. 2019
*//*-----------------------------------------------------------------------------------*/
extern bool gendev_GetSubscriptionByIndex_bol(uint16_t idx_u16,
                                                mqttif_substParam_t *dest_stp)
{
    bool indexIsValid_bol = false;

    if(this_sst.subsCounter_u16 > idx_u16)
    {
        indexIsValid_bol = true;
        dest_stp->conn_fp = OnConnectionHandler_vd;
        dest_stp->discon_fp = OnDisconnectionHandler_vd;
        dest_stp->dataRecv_fp = OnDataReceivedHandler_st;
        dest_stp->qos_u32 = 0;

        memset(&dest_stp->topic_u8a[0], 0U, sizeof(dest_stp->topic_u8a));
        memcpy(&dest_stp->topic_u8a[0], &this_sst.subs_chap[idx_u16][0],
             utils_MIN(strlen(this_sst.subs_chap[idx_u16]),
                         mqttif_MAX_SIZE_OF_TOPIC));
    }
    else
    {
        indexIsValid_bol = false;
    }

    return(indexIsValid_bol);
}

/**---------------------------------------------------------------------------------------
 * @brief     Activate the generic device function
 * @author    S. Wink
 * @date      25. Mar. 2019
*//*-----------------------------------------------------------------------------------*/
esp_err_t gendev_Activate_st(void)
{
    esp_err_t result_st = ESP_FAIL;

    if(STATE_INITIALIZED == this_sst.state_en)
    {
        if(NULL != this_sst.cycleTimer_st)
        {
            if(pdPASS == xTimerStart(this_sst.cycleTimer_st, 0))
            {
                result_st = ESP_OK;
                this_sst.state_en = STATE_ACTIVE;
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
 * @brief     Deacrivates the generic device function
 * @author    S. Wink
 * @date      25. Mar. 2019
*//*-----------------------------------------------------------------------------------*/
esp_err_t gendev_Deactivate_st(void)
{
    esp_err_t result_st = ESP_FAIL;

    if(STATE_NOT_INITIALIZED != this_sst.state_en)
    {
        if(NULL != this_sst.cycleTimer_st)
        {
            if(pdPASS == xTimerStop(this_sst.cycleTimer_st, 0))
            {
                result_st = ESP_OK;
                this_sst.state_en = STATE_DEACTIVATED;
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
    xEventGroupSetBits(this_sst.eventGroup_st, MQTT_CONNECT);
}

/**---------------------------------------------------------------------------------------
 * @brief     Handler when disconnected from mqtt broker
 * @author    S. Wink
 * @date      08. Apr. 2019
 * @return    n/a
*//*-----------------------------------------------------------------------------------*/
static void OnDisconnectionHandler_vd(void)
{
    xEventGroupSetBits(this_sst.eventGroup_st, MQTT_DISCONNECT);
}

/**--------------------------------------------------------------------------------------
 * @brief     function to send firmware info record
 * @author    S. Wink
 * @date      29. May. 2019
 * @return    n/a
*//*-----------------------------------------------------------------------------------*/
static void SendInfoRecord_vd(void)
{
    bool exeResult_bol = true;
    mqttif_Publish_td publisher_td = this_sst.param_st.publishHandler_fp;


    ESP_LOGD(TAG, "send firmware identifier...");
    utils_BuildSendTopic_chp(this_sst.param_st.deviceName_chp, this_sst.param_st.id_chp,
                                MQTT_PUB_FW_IDENT, this_sst.pubMsg_st.topic_chp);

    this_sst.pubMsg_st.topicLen_u32 = strlen(this_sst.pubMsg_st.topic_chp);
    this_sst.pubMsg_st.dataLen_u32 =
        sprintf(this_sst.pubMsg_st.data_chp, "Firmware PN: %s", 
                    appIdent_GetFwIdentifier_cch());
    ESP_LOGD(TAG, "topic:%s, data:%s", this_sst.pubMsg_st.topic_chp, 
                this_sst.pubMsg_st.data_chp);
    exeResult_bol &= CHECK_EXE(publisher_td(&this_sst.pubMsg_st, MAX_PUB_WAIT));
    ESP_LOGD(TAG, "send firmware version...");
    if(true == exeResult_bol)
    {
        utils_BuildSendTopic_chp(this_sst.param_st.deviceName_chp,
                                    this_sst.param_st.id_chp, MQTT_PUB_FW_VERSION,
                                    this_sst.pubMsg_st.topic_chp);
        this_sst.pubMsg_st.topicLen_u32 = strlen(this_sst.pubMsg_st.topic_chp);
        this_sst.pubMsg_st.dataLen_u32 =
            sprintf(this_sst.pubMsg_st.data_chp, "Firmware Version: %s",
                            appIdent_GetFwVersion_cch());
        ESP_LOGD(TAG, "topic:%s, data:%s", this_sst.pubMsg_st.topic_chp,
                    this_sst.pubMsg_st.data_chp);
        exeResult_bol &= CHECK_EXE(publisher_td(&this_sst.pubMsg_st, MAX_PUB_WAIT));
    }

    ESP_LOGD(TAG, "send firmware description...");
    if(true == exeResult_bol)
    {
        utils_BuildSendTopic_chp(this_sst.param_st.deviceName_chp,
                                    this_sst.param_st.id_chp, MQTT_PUB_FW_DESC,
                                    this_sst.pubMsg_st.topic_chp);
        this_sst.pubMsg_st.topicLen_u32 = strlen(this_sst.pubMsg_st.topic_chp);
        this_sst.pubMsg_st.dataLen_u32 =
            sprintf(this_sst.pubMsg_st.data_chp, "Firmware Description: %s",
                        appIdent_GetFwDescription_cch());
        ESP_LOGD(TAG, "topic:%s, data:%s", this_sst.pubMsg_st.topic_chp,
                    this_sst.pubMsg_st.data_chp);
        CHECK_EXE(publisher_td(&this_sst.pubMsg_st, MAX_PUB_WAIT));
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

    if(   (0U == strncmp(msg_stp->topic_chp, &this_sst.subs_chap[0][0],
                            msg_stp->topicLen_u32))
       || (0U == strncmp(msg_stp->topic_chp, &this_sst.subs_chap[1][0],
                            msg_stp->topicLen_u32)))
    {
        if(   (0U != msg_stp->dataLen_u32)
           && (0U == strncmp(msg_stp->data_chp, MQTT_PAYLOAD_CMD_INFO,
                                msg_stp->dataLen_u32)))
        {
            // generic info command received, notify task for publishing
            // information record
            xEventGroupSetBits(this_sst.eventGroup_st, MQTT_INFO_REQUEST);
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
    utils_BuildSendTopic_chp(this_sst.param_st.deviceName_chp, this_sst.param_st.id_chp,
                                MQTT_PUB_HEALTH, this_sst.pubMsg_st.topic_chp);
    this_sst.pubMsg_st.topicLen_u32 = strlen(this_sst.pubMsg_st.topic_chp);
    this_sst.pubMsg_st.dataLen_u32 = sprintf(this_sst.pubMsg_st.data_chp, 
                                                "%d",this_sst.healthCounter_u32);
    CHECK_EXE(this_sst.param_st.publishHandler_fp(&this_sst.pubMsg_st, MAX_PUB_WAIT));
    ESP_LOGD(TAG, "publish: %s :: %s", this_sst.pubMsg_st.topic_chp, 
                this_sst.pubMsg_st.data_chp);
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
    xEventGroupSetBits(this_sst.eventGroup_st, CYCLE_TIMER);
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
        uxBits_st = xEventGroupWaitBits(this_sst.eventGroup_st, bits_u32,
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
            this_sst.healthCounter_u32++;
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

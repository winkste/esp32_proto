/*****************************************************************************************
* FILENAME :        mqttdrv.c
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
*
*****************************************************************************************/

/****************************************************************************************/
/* Include Interfaces */
#include "mqttdrv.h"

#include "stdbool.h"
#include "stddef.h"
#include "string.h"
#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "mqtt_client.h"

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

typedef enum objectState_tag
{
     STATE_NOT_INITIALIZED,
     STATE_INITIALIZED,
     STATE_CONNECT_IN_PROGRESS,
     STATE_CONNECTED,
     STATE_DISCONNECTED
}objectState_t;

typedef struct mqttdrv_subsObj_tag
{
     mqttdrv_substParam_t param_st;
     bool subscribed_bol;
     mqttdrv_subsHdl_t next_xp;
     mqttdrv_subsHdl_t last_xp;
}mqttdrv_subsObj_t;

typedef struct objectData_tag
{
     objectState_t state_en;
     esp_mqtt_client_handle_t client_xp;
     mqttdrv_subsHdl_t subst_xp;
}objectData_t;

/****************************************************************************************/
/* Local functions prototypes: */
static void AddSubsToList_vd(mqttdrv_subsHdl_t subsHdl_xp);
static void RemoveSubsFromList_vd(mqttdrv_subsHdl_t subsHdl_xp);
static esp_err_t MqttEventHandler_st(esp_mqtt_event_handle_t event_stp);
static void HandleConnect_vd(esp_mqtt_event_handle_t event_stp);
static void HandleDisconnect_vd(esp_mqtt_event_handle_t event_stp);
static void HandleSubscription_vd(esp_mqtt_event_handle_t event_stp);
static void HandleUnsubscription_vd(esp_mqtt_event_handle_t event_stp);
static void HandlePublish_vd(esp_mqtt_event_handle_t event_stp);
static void HandleData_vd(esp_mqtt_event_handle_t event_stp);
static void HandleError_vd(esp_mqtt_event_handle_t event_stp);
static esp_err_t Connect(void);
static esp_err_t Disconnect(void);
static void Task_vd(void *pvParameters);

/****************************************************************************************/
/* Local variables: */

static const char *TAG = "mqttdrv";

static const int START_MQTT    = BIT0;
static const int STOP_MQTT     = BIT1;
static const int PUBLISH_REQ   = BIT2;
static const int MQTT_ERR      = BIT3;

static objectData_t singleton_sst =
{
        .state_en = STATE_NOT_INITIALIZED,
        .client_xp = NULL,
        .subst_xp = NULL,
};

static EventGroupHandle_t mqttEventGroup_sts;

static SemaphoreHandle_t publishSema_sts;

static char topicVector_sca[mqttif_MAX_SIZE_OF_TOPIC];
static char dataVector_sca[mqttif_MAX_SIZE_OF_DATA];
static mqttif_msg_t pubMsg_sts;
/****************************************************************************************/
/* Global functions (unlimited visibility) */

/**---------------------------------------------------------------------------------------
 * @brief     Initializes the initialization structure of the mqtt module
*//*-----------------------------------------------------------------------------------*/
esp_err_t mqttdrv_InitializeParameter(mqttdrv_param_t *param_stp)
{
    memset(param_stp, 0U, sizeof(*param_stp));

    return(ESP_OK);
}

/**---------------------------------------------------------------------------------------
 * @brief     Initialization of the mqtt module
*//*-----------------------------------------------------------------------------------*/
esp_err_t mqttdrv_Initialize(mqttdrv_param_t *param_stp)
{
    esp_err_t result_st = ESP_FAIL;
    esp_mqtt_client_config_t mqttCfg_st;

    esp_log_level_set(TAG, ESP_LOG_DEBUG);
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_WARN);
    esp_log_level_set("OUTBOX", ESP_LOG_WARN);

    ESP_LOGD(TAG, "initialize mqtt...");

    if(     (STATE_DISCONNECTED == singleton_sst.state_en)
         || (STATE_NOT_INITIALIZED == singleton_sst.state_en))
    {
        if(NULL != param_stp)
        {
            /* reset the mqtt configuration structure to 0 first,
             * or it will generate crashes
             */
            memset(&mqttCfg_st, 0, sizeof(mqttCfg_st));
            mqttCfg_st.host = (char *)&param_stp->host_u8a[0];
            mqttCfg_st.port = param_stp->port_u32;
            mqttCfg_st.username = (char *)&param_stp->userName_u8a[0];
            mqttCfg_st.password = (char *)&param_stp->userPwd_u8a[0];
            mqttCfg_st.event_handle = MqttEventHandler_st;

            // cleanup the client before initialize to ensure old conncetions are closed
            if(NULL != singleton_sst.client_xp)
            {
                (void)esp_mqtt_client_destroy(singleton_sst.client_xp);
            }

            ESP_LOGD(TAG, "connect to %s, port:%d",mqttCfg_st.host, mqttCfg_st.port);
            ESP_LOGD(TAG, "usr:%s, pwd:%s", mqttCfg_st.username, mqttCfg_st.password);
            singleton_sst.client_xp = esp_mqtt_client_init(&mqttCfg_st);
            if(NULL != singleton_sst.client_xp)
            {
                result_st = ESP_OK;
                singleton_sst.state_en = STATE_INITIALIZED;
                ESP_LOGD(TAG, "mqtt client initialized successful...");
            }
            else
            {
                result_st = ESP_FAIL;
                ESP_LOGE(TAG, "mqtt client init failed...");
            }
        }
        else
        {
            result_st = ESP_FAIL;
            ESP_LOGE(TAG, "mqtt client init parameter failure...");
        }
    }
    else
    {
        result_st = ESP_FAIL;
        ESP_LOGE(TAG, "mqtt client init wrong state detected...");
    }

    publishSema_sts = xSemaphoreCreateBinary();
    if(NULL == publishSema_sts)
    {
        result_st = ESP_FAIL;
        ESP_LOGE(TAG, "mqtt client init sema alloc failed...");
    }
    else
    {
        xSemaphoreGive(publishSema_sts);
    }

    mqttEventGroup_sts = xEventGroupCreate();
    if(NULL == mqttEventGroup_sts)
    {
        result_st = ESP_FAIL;
        ESP_LOGE(TAG, "mqtt client init event alloc failed...");
    }

    /* start the mqtt task */
    xTaskCreate(Task_vd, "mqttTask", 4096, NULL, 4, NULL);

    ESP_LOGD(TAG, "initialize mqtt done...");
    return(result_st);
}

/**---------------------------------------------------------------------------------------
 * @brief     Starts the MQTT demon if it was initialized
*//*-----------------------------------------------------------------------------------*/
void mqttdrv_StartMqttDemon(void)
{
    if(STATE_INITIALIZED == singleton_sst.state_en)
    {
        xEventGroupSetBits(mqttEventGroup_sts, START_MQTT);
    }
}

/**---------------------------------------------------------------------------------------
 * @brief     Initialize subscription parameter
*//*-----------------------------------------------------------------------------------*/
esp_err_t mqttdrv_InitSubscriptParam(mqttif_substParam_t *subsParam_stp)
{
    esp_err_t result_st = ESP_OK;

    if(NULL != subsParam_stp)
    {
        memset(subsParam_stp, 0U, sizeof(mqttdrv_substParam_t));
    }
    else
    {
        ESP_LOGW(TAG, "mqtt client init subscription param NULL pointer...");
    }
    return(result_st);
}

/**---------------------------------------------------------------------------------------
 * @brief     Allocate subscribe handle
*//*-----------------------------------------------------------------------------------*/
mqttdrv_subsHdl_t mqttdrv_AllocSubs_xp(mqttif_substParam_t *subsParam_stp)
{
    mqttdrv_subsHdl_t handle_xp;

    if(NULL != subsParam_stp)
    {
        handle_xp = malloc(sizeof(mqttdrv_subsObj_t));
        if(NULL != handle_xp)
        {
            // initialize the handle data
            memcpy(&handle_xp->param_st, subsParam_stp, sizeof(handle_xp->param_st));
            handle_xp->subscribed_bol = false;
            handle_xp->next_xp = NULL;

            // add the handle to the list, either as first element or at the end
            AddSubsToList_vd(handle_xp);

            ESP_LOGD(TAG, "topic %s, subscribed", subsParam_stp->topic_u8a);
        }
    }
    else
    {
        handle_xp = NULL;
    }

    if(NULL == handle_xp)
    {
        ESP_LOGE(TAG, "subscription handle allocation failed...");
    }

    return(handle_xp);
}

/**---------------------------------------------------------------------------------------
 * @brief     Deallocate subscribe handle
*//*-----------------------------------------------------------------------------------*/
esp_err_t mqttdrv_DeAllocSubs_st(mqttdrv_subsHdl_t subsHdl_xp)
{
    esp_err_t result_st = ESP_FAIL;

    if(NULL != subsHdl_xp)
    {
        if(true == subsHdl_xp->subscribed_bol)
        {
            // topic is still subscribed, first remove the subscription
            result_st = mqttdrv_UnSubscribe(subsHdl_xp);

            // now, remove subscription entry from list
            RemoveSubsFromList_vd(subsHdl_xp);
        }
    }

    if(ESP_FAIL == result_st)
    {
        ESP_LOGE(TAG, "deallocation of subscription failed...");
    }

    return(result_st);
}

/**---------------------------------------------------------------------------------------
 * @brief     Subscribe to a MQTT message
*//*-----------------------------------------------------------------------------------*/
esp_err_t mqttdrv_Subscribe_xp(mqttdrv_subsHdl_t subsHdl_xp)
{
    esp_err_t result_st = ESP_FAIL;

    if(NULL != subsHdl_xp)
    {
        result_st = esp_mqtt_client_subscribe(singleton_sst.client_xp,
                                              (char *)&subsHdl_xp->param_st.topic_u8a[0],
                                              subsHdl_xp->param_st.qos_u32);
        if(ESP_OK == result_st)
        {
            ESP_LOGD(TAG, "subscribed to topic: %s",
                                (char *)&subsHdl_xp->param_st.topic_u8a[0]);
            subsHdl_xp->subscribed_bol = true;
        }
    }
    else
    {
        result_st = ESP_FAIL;
    }

    if(ESP_FAIL == result_st)
    {
        ESP_LOGE(TAG, "subscription failed...");
    }

    return(result_st);
}

/**---------------------------------------------------------------------------------------
 * @brief     Un-Subscribe MQTT message
*//*-----------------------------------------------------------------------------------*/
esp_err_t mqttdrv_UnSubscribe(mqttdrv_subsHdl_t subsHdl_xp)
{
    esp_err_t result_st = ESP_FAIL;

    if(NULL != subsHdl_xp)
    {
        result_st = esp_mqtt_client_unsubscribe(singleton_sst.client_xp,
                                             (char *)&subsHdl_xp->param_st.topic_u8a[0]);
        if(ESP_OK == result_st)
        {
            subsHdl_xp->subscribed_bol = false;
        }
    }
    else
    {
        result_st = ESP_FAIL;
    }

    if(ESP_FAIL == result_st)
    {
        ESP_LOGE(TAG, "un-subscription failed...");
    }

    return(result_st);
}

/**---------------------------------------------------------------------------------------
 * @brief     Get the number of all allocated subscriptions in the list
*//*-----------------------------------------------------------------------------------*/
uint8_t mqttdrv_GetNumberOfSubscriptions(void)
{
    uint8_t counter_u8 = 0U;
    mqttdrv_subsHdl_t index_xps = singleton_sst.subst_xp;

    while(NULL != index_xps)
    {
        counter_u8++;
        index_xps = index_xps->next_xp;
    }

    return(counter_u8);
}

/**---------------------------------------------------------------------------------------
 * @brief     Un-Subscribe MQTT message
*//*-----------------------------------------------------------------------------------*/
esp_err_t mqttdrv_Publish_st(mqttif_msg_t *msg_stp, uint32_t timeOut_u32)
{
    esp_err_t result_st = ESP_FAIL;

    ESP_LOGD(TAG, "publish request function called...");

    if(NULL == publishSema_sts)
        ESP_LOGE(TAG, "publish sema is null");
    if(NULL == msg_stp)
            ESP_LOGE(TAG, "publish msg is null");
    if(mqttif_MAX_SIZE_OF_DATA < msg_stp->dataLen_u32)
        ESP_LOGE(TAG, "publish msg data length max reached");
    if(mqttif_MAX_SIZE_OF_TOPIC < msg_stp->topicLen_u32)
            ESP_LOGE(TAG, "publish msg topic length max: %d", msg_stp->topicLen_u32);

    if(   (NULL != publishSema_sts)
       && (NULL != msg_stp)
       && (mqttif_MAX_SIZE_OF_DATA > msg_stp->dataLen_u32)
       && (mqttif_MAX_SIZE_OF_TOPIC > msg_stp->topicLen_u32))
    {
        ESP_LOGD(TAG, "publish msg parameter check ok...");
        /* See if we can obtain the semaphore.  If the semaphore is not
        available wait to see if it becomes free. */
        if(pdTRUE == xSemaphoreTake(publishSema_sts, ( TickType_t ) timeOut_u32))
        {
            /* We were able to obtain the semaphore and can now access the
            shared resource. */
            ESP_LOGD(TAG, "publish sema obtained...");
            memset(topicVector_sca, 0x00, mqttif_MAX_SIZE_OF_TOPIC);
            memset(dataVector_sca, 0x00, mqttif_MAX_SIZE_OF_TOPIC);
            pubMsg_sts.topic_chp = topicVector_sca;
            pubMsg_sts.data_chp = dataVector_sca;
            pubMsg_sts.topicLen_u32 = msg_stp->topicLen_u32;
            pubMsg_sts.dataLen_u32 = msg_stp->dataLen_u32;
            memcpy(pubMsg_sts.topic_chp, msg_stp->topic_chp, pubMsg_sts.topicLen_u32);
            memcpy(pubMsg_sts.data_chp, msg_stp->data_chp, pubMsg_sts.dataLen_u32);
            pubMsg_sts.msgId_s32 = 0;
            pubMsg_sts.qos_s32 = msg_stp->qos_s32;
            pubMsg_sts.retain_s32 = msg_stp->qos_s32;

            xEventGroupSetBits(mqttEventGroup_sts, PUBLISH_REQ);
            result_st = ESP_OK;
        }
        else
        {
            /* We could not obtain the semaphore and can therefore not access
            the shared resource safely. */
            ESP_LOGW(TAG, "publish sema request failed...");
        }
    }
    return(result_st);
}

/****************************************************************************************/
/* Local functions: */

/**---------------------------------------------------------------------------------------
 * @brief     Add the subscription object handle to the list for later handling
 * @author    S. Wink
 * @date      24. Jan. 2019
 * @param     subsHdl_xp        subscription handler
 * @return    n/a
*//*------------------------------------------------------------------------------------*/
static void AddSubsToList_vd(mqttdrv_subsHdl_t subsHdl_xp)
{
    if(NULL == singleton_sst.subst_xp)
    {
        singleton_sst.subst_xp = subsHdl_xp;
        singleton_sst.subst_xp->next_xp = NULL;
    }
    else
    {
        mqttdrv_subsHdl_t current_xp = singleton_sst.subst_xp;
        while(NULL != current_xp->next_xp)
        {
            current_xp = current_xp->next_xp;
        }
        current_xp->next_xp = subsHdl_xp;
    }
}

/**---------------------------------------------------------------------------------------
 * @brief     Add the subscription object handle to the list for later handling
 * @author    S. Wink
 * @date      24. Jan. 2019
 * @param     subsHdl_xp        subscription handler
 * @return    n/a
*//*------------------------------------------------------------------------------------*/
static void RemoveSubsFromList_vd(mqttdrv_subsHdl_t subsHdl_xp)
{
    mqttdrv_subsHdl_t last_xp = NULL;
    mqttdrv_subsHdl_t current_xp;

    if((NULL != subsHdl_xp) && (NULL != singleton_sst.subst_xp))
    {
        last_xp = singleton_sst.subst_xp;
        current_xp = singleton_sst.subst_xp->next_xp;

        while(NULL != current_xp)
        {
            if(subsHdl_xp == current_xp)
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
}

/**---------------------------------------------------------------------------------------
 * @brief     Event handling for MQTT events
 * @author    S. Wink
 * @date      24. Jan. 2019
 * @param     event_stp        event structure
 * @return    n/a
*//*------------------------------------------------------------------------------------*/
static esp_err_t MqttEventHandler_st(esp_mqtt_event_handle_t event_stp)
{

    switch (event_stp->event_id)
    {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGD(TAG, "MQTT_EVENT_CONNECTED");
            HandleConnect_vd(event_stp);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGD(TAG, "MQTT_EVENT_DISCONNECTED");
            HandleDisconnect_vd(event_stp);
            break;
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGD(TAG, "MQTT_EVENT_SUBSCRIBED");
            HandleSubscription_vd(event_stp);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGD(TAG, "MQTT_EVENT_UNSUBSCRIBED");
            HandleUnsubscription_vd(event_stp);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGD(TAG, "MQTT_EVENT_PUBLISHED");
            HandlePublish_vd(event_stp);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGD(TAG, "MQTT_EVENT_DATA");
            HandleData_vd(event_stp);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGD(TAG, "MQTT_EVENT_ERROR");
            HandleError_vd(event_stp);
            break;
        default:
            ESP_LOGD(TAG, "default");
            HandleError_vd(event_stp);
            break;
    }
    return ESP_OK;
}

/**---------------------------------------------------------------------------------------
 * @brief     Handler for subscription event
 * @author    S. Wink
 * @date      31. Mar. 2019
 * @param     event_stp        event structure
 * @return    n/a
*//*------------------------------------------------------------------------------------*/
static void HandleSubscription_vd(esp_mqtt_event_handle_t event_stp)
{
    //ESP_LOGI(TAG, "handle subscription event, msg_id=%d", event_stp->msg_id);
}

/**---------------------------------------------------------------------------------------
 * @brief     Handler for un-subscription event
 * @author    S. Wink
 * @date      31. Mar. 2019
 * @param     event_stp        event structure
 * @return    n/a
*//*------------------------------------------------------------------------------------*/
static void HandleUnsubscription_vd(esp_mqtt_event_handle_t event_stp)
{
    //ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event_stp->msg_id);
}

/**---------------------------------------------------------------------------------------
 * @brief     Handler for connection event
 * @author    S. Wink
 * @date      31. Mar. 2019
 * @param     event_stp        event structure
 * @return    n/a
*//*------------------------------------------------------------------------------------*/
static void HandleConnect_vd(esp_mqtt_event_handle_t event_stp)
{
    esp_err_t result_st = ESP_FAIL;

    if((STATE_DISCONNECTED == singleton_sst.state_en) ||
       (STATE_CONNECT_IN_PROGRESS == singleton_sst.state_en))
    {
        singleton_sst.state_en = STATE_CONNECTED;
        ESP_LOGI(TAG, "mqtt connected...");

        // TODO message to control task that MQTT is online

        mqttdrv_subsHdl_t index_xps = singleton_sst.subst_xp;
        while(NULL != index_xps)
        {
            //subscribe users
            result_st = esp_mqtt_client_subscribe(singleton_sst.client_xp,
                    (char *)&index_xps->param_st.topic_u8a[0],
                    index_xps->param_st.qos_u32);
            ESP_LOGI(TAG, "subscribed to topic: %s, result:%d",
                    (char *)&index_xps->param_st.topic_u8a[0], result_st);

            // call all connect functions of registered users
            if(NULL != index_xps->param_st.conn_fp)
            {
                index_xps->param_st.conn_fp();
            }

            index_xps = index_xps->next_xp;

        }
    }
    else
    {
        ESP_LOGE(TAG, "unexpected mqtt state reached...");
        Disconnect();
    }
}

/**---------------------------------------------------------------------------------------
 * @brief     Handler for disconnection event
 * @author    S. Wink
 * @date      31. Mar. 2019
 * @param     event_stp        event structure
 * @return    n/a
*//*------------------------------------------------------------------------------------*/
static void HandleDisconnect_vd(esp_mqtt_event_handle_t event_stp)
{
    if((STATE_CONNECTED == singleton_sst.state_en) ||
       (STATE_INITIALIZED == singleton_sst.state_en))
    {
        singleton_sst.state_en = STATE_DISCONNECTED;
        ESP_LOGI(TAG, "mqtt disconnected...");

        // TODO: message to control task that MQTT is offline

        // call all registered callback functions for disconnection event
        mqttdrv_subsHdl_t index_xps = singleton_sst.subst_xp;

        while(NULL != index_xps)
        {
            if(NULL != index_xps->param_st.discon_fp)
            {
                index_xps->param_st.discon_fp();
            }
            index_xps = index_xps->next_xp;
        }
    }
    else if(STATE_NOT_INITIALIZED == singleton_sst.state_en)
    {
        ESP_LOGW(TAG, "disconnection before initialization...");
    }
    else
    {
        ESP_LOGW(TAG, "unexpected state reached while disconnection...");
    }
}

/**---------------------------------------------------------------------------------------
 * @brief     Handler for publication event
 * @author    S. Wink
 * @date      31. Mar. 2019
 * @param     event_stp        event structure
 * @return    n/a
*//*------------------------------------------------------------------------------------*/
static void HandlePublish_vd(esp_mqtt_event_handle_t event_stp)
{

    if(event_stp->msg_id == pubMsg_sts.msgId_s32)
    {
        ESP_LOGD(TAG, "publication as requested complete, msg_id=%d", event_stp->msg_id);
    }
    else
    {
        ESP_LOGW(TAG, "unexpected publication:, msg_id=%d", event_stp->msg_id);
    }

    /* We have finished accessing the shared resource.  Release the
    semaphore. */
    ESP_LOGD(TAG, "sema released...");
    xSemaphoreGive(publishSema_sts);
}

/**---------------------------------------------------------------------------------------
 * @brief     Handler for data received event
 * @author    S. Wink
 * @date      31. Mar. 2019
 * @param     event_stp        event structure
 * @return    n/a
*//*------------------------------------------------------------------------------------*/
static void HandleData_vd(esp_mqtt_event_handle_t event_stp)
{
    mqttdrv_subsHdl_t index_xps = singleton_sst.subst_xp;
    mqttif_msg_t msg_st;

    msg_st.topic_chp = event_stp->topic;
    msg_st.topicLen_u32 = event_stp->topic_len;
    msg_st.dataLen_u32 = event_stp->data_len;
    msg_st.data_chp = event_stp->data;

    ESP_LOGD(TAG, "topic=%.*s, length: %d", msg_st.topicLen_u32, msg_st.topic_chp,
            msg_st.topicLen_u32);
    ESP_LOGD(TAG, "data=%.*s, length: %d", msg_st.dataLen_u32, msg_st.data_chp,
            msg_st.dataLen_u32);

    if(NULL != index_xps)
    {
        while(NULL != index_xps)
        {
            if(0U == strncmp(event_stp->topic,
                             (char *)&index_xps->param_st.topic_u8a[0],
                             event_stp->topic_len))
            {
                if(NULL !=  index_xps->param_st.dataRecv_fp)
                {
                    index_xps->param_st.dataRecv_fp(&msg_st);
                }
                else
                {
                    ESP_LOGW(TAG, "subscription received without callback function");
                }
            }
            index_xps = index_xps->next_xp;
        }
    }
}

/**---------------------------------------------------------------------------------------
 * @brief     Handler for error event
 * @author    S. Wink
 * @date      31. Mar. 2019
 * @param     event_stp        event structure
 * @return    n/a
*//*------------------------------------------------------------------------------------*/
static void HandleError_vd(esp_mqtt_event_handle_t event_stp)
{
    ESP_LOGE(TAG, "MQTT_EVENT_ERROR detected...");

    // ensure that the publication resource gets released
    xSemaphoreGive(publishSema_sts);
    xEventGroupSetBits(mqttEventGroup_sts, MQTT_ERR);
}

/**---------------------------------------------------------------------------------------
 * @brief     Connects to the MQTT broker
 * @author    S. Wink
 * @date      25. Mar. 2019
 * @return    n/a
*//*-----------------------------------------------------------------------------------*/
static esp_err_t Connect(void)
{
    esp_err_t result_st = ESP_FAIL;

    switch(singleton_sst.state_en)
    {
        case STATE_INITIALIZED:
        case STATE_DISCONNECTED:
            if(NULL != singleton_sst.client_xp)
            {
                result_st = esp_mqtt_client_start(singleton_sst.client_xp);
                ESP_LOGD(TAG, "client connection done...");
                singleton_sst.state_en = STATE_CONNECT_IN_PROGRESS;
            }
            else
            {
                result_st = ESP_FAIL;
                ESP_LOGE(TAG, "connection failed...");
            }
            break;
        case STATE_CONNECTED:
        case STATE_CONNECT_IN_PROGRESS:
            result_st = ESP_OK;
            break;
        default:
            result_st = ESP_FAIL;
            ESP_LOGE(TAG, "unexpected state during connection...");
            break;
    }

    return(result_st);
}

/**---------------------------------------------------------------------------------------
 * @brief     Disconnects from the MQTT broker
 * @author    S. Wink
 * @date      25. Mar. 2019
 * @return    n/a
*//*-----------------------------------------------------------------------------------*/
static esp_err_t Disconnect(void)
{
    esp_err_t result_st = ESP_FAIL;

    if(   (STATE_NOT_INITIALIZED != singleton_sst.state_en)
       && (STATE_INITIALIZED != singleton_sst.state_en))
    {
        singleton_sst.state_en = STATE_DISCONNECTED;
        result_st = esp_mqtt_client_stop(singleton_sst.client_xp);

    }
    else
    {
        result_st = ESP_FAIL;
    }

    return(result_st);
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
    uint32_t bits_u32 = START_MQTT | STOP_MQTT | PUBLISH_REQ | MQTT_ERR;

    ESP_LOGD(TAG, "mqttTask started...");
    while(1)
    {
        uxBits_st = xEventGroupWaitBits(mqttEventGroup_sts, bits_u32,
                                         true, false, portMAX_DELAY); // @suppress("Symbol is not resolved")

        if(0 != (uxBits_st & START_MQTT))
        {
            //ESP_LOGI(TAG, "connect request...");
            (void)Connect();
        }
        if(0 != (uxBits_st & STOP_MQTT))
        {
            (void)Disconnect();
        }
        if(0 != (uxBits_st & PUBLISH_REQ))
        {
            ESP_LOGD(TAG, "publish start...");
            pubMsg_sts.msgId_s32 = esp_mqtt_client_publish(singleton_sst.client_xp,
                                                            pubMsg_sts.topic_chp,
                                                            pubMsg_sts.data_chp,
                                                            pubMsg_sts.dataLen_u32,
                                                            pubMsg_sts.qos_s32,
                                                            pubMsg_sts.retain_s32);
            if((-1 == pubMsg_sts.msgId_s32))
            {
                // message publication failed
                xSemaphoreGive(publishSema_sts);
                ESP_LOGE(TAG, "error during publication");
            }
            else if(0 == pubMsg_sts.qos_s32)
            {
                // we don't need a message send confirmation
                xSemaphoreGive(publishSema_sts);
            }
        }
        if(0 != (uxBits_st & MQTT_ERR))
        {

        }
    }
}



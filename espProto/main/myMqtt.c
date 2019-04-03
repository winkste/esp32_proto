/*****************************************************************************************
* FILENAME :        myMqtt.c
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
#include "myMqtt.h"

#include "stdbool.h"
#include "stddef.h"
#include "string.h"
#include "esp_log.h"
#include "esp_err.h"
#include "mqtt_client.h"

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
     STATE_CONNECT_IN_PROGRESS,
     STATE_CONNECTED,
     STATE_DISCONNECTED
 }moduleState_t;

 typedef struct myMqtt_obj_tag
 {
     myMqtt_substParam_t param_st;
     bool subscribed_bol;
     mqtt_subsHdl_t next_xp;
     mqtt_subsHdl_t last_xp;
 }myMqtt_obj_t;

/****************************************************************************************/
/* Local functions prototypes: */
 static void AddSubsToList_vd(mqtt_subsHdl_t subsHdl_xp);
 static void RemoveSubsFromList_vd(mqtt_subsHdl_t subsHdl_xp);
 static bool IsListEmpty_bol(void);

 static void HandleConnect_vd(esp_mqtt_event_handle_t event_stp);
 static void HandleDisconnect_vd(esp_mqtt_event_handle_t event_stp);
 static void HandleSubscription_vd(esp_mqtt_event_handle_t event_stp);
 static void HandleUnsubscription_vd(esp_mqtt_event_handle_t event_stp);
 static void HandlePublish_vd(esp_mqtt_event_handle_t event_stp);
 static void HandleData_vd(esp_mqtt_event_handle_t event_stp);
 static void HandleError_vd(esp_mqtt_event_handle_t event_stp);

 static esp_err_t MqttEventHandler_st(esp_mqtt_event_handle_t event_stp);

/****************************************************************************************/
/* Local variables: */

static const char *TAG = "myMqtt";
static moduleState_t moduleState_ens = STATE_NOT_INITIALIZED;
static esp_mqtt_client_handle_t client_xps = NULL;
static mqtt_subsHdl_t list_xps;
/****************************************************************************************/
/* Global functions (unlimited visibility) */

/**---------------------------------------------------------------------------------------
 * @brief     Initializes the initialization structure of the mqtt module
*//*-----------------------------------------------------------------------------------*/
esp_err_t myMqtt_InitializeParameter(myMqtt_param_t *param_stp)
{
    memset(param_stp, 0U, sizeof(*param_stp));

    ESP_LOGI(TAG, "parameter for module initialization setup...");

    return(ESP_OK);
}

/**---------------------------------------------------------------------------------------
 * @brief     Initialization of the mqtt module
*//*-----------------------------------------------------------------------------------*/
esp_err_t myMqtt_Initialize(myMqtt_param_t *param_stp)
{
    esp_err_t result_st = ESP_FAIL;
    esp_mqtt_client_config_t mqttCfg_st;

    ESP_LOGI(TAG, "initialize mqtt...");

    if(     (STATE_DISCONNECTED == moduleState_ens)
         || (STATE_NOT_INITIALIZED == moduleState_ens))
    {
        if(NULL != param_stp)
        {
            mqttCfg_st.host = (char *)&param_stp->host_u8a[0];
            mqttCfg_st.port = param_stp->port_u32;
            mqttCfg_st.username = (char *)&param_stp->userName_u8a[0];
            mqttCfg_st.password = (char *)&param_stp->userPwd_u8a[0];
            mqttCfg_st.event_handle = MqttEventHandler_st;
            ESP_LOGI(TAG, "connect to %s, port:%d",mqttCfg_st.host, mqttCfg_st.port);
            ESP_LOGI(TAG, "usr:%s, pwd:%s", mqttCfg_st.username, mqttCfg_st.password);
            if(NULL != client_xps)
            {
                (void)esp_mqtt_client_destroy(client_xps);
            }

            ESP_LOGI(TAG, "mqtt client try to init...");
            //client_xps = esp_mqtt_client_init(&mqttCfg_st);
            const esp_mqtt_client_config_t mqtt_cfg = {
                    //.uri = "mqtt://iot.eclipse.org",
                    .host = mqttCfg_st.host,//"192.168.178.45",
                    .port = mqttCfg_st.port, //1883,
                    .username = mqttCfg_st.username, //"winkste",
                    .password = mqttCfg_st.password,//"sw10950",
                    .event_handle = MqttEventHandler_st,
                    // .user_context = (void *)your_context
                };
            client_xps = esp_mqtt_client_init(&mqtt_cfg);
            if(NULL != client_xps)
            {
                result_st = ESP_OK;
                moduleState_ens = STATE_INITIALIZED;
                ESP_LOGI(TAG, "mqtt client initialized...");
            }
            else
            {
                result_st = ESP_FAIL;
                ESP_LOGI(TAG, "mqtt client init failed...");
            }

            // initialize the subscription list
            if(!IsListEmpty_bol())
            {
                result_st = ESP_FAIL;
            }
        }
        else
        {
            result_st = ESP_FAIL;
        }
    }
    else
    {
        result_st = ESP_FAIL;
    }

    return(result_st);
}

/**---------------------------------------------------------------------------------------
 * @brief     Connects to the MQTT broker
 * @author    S. Wink
 * @date      25. Mar. 2019
 * @return    n/a
*//*-----------------------------------------------------------------------------------*/
esp_err_t myMqtt_Connect(void)
{
    esp_err_t result_st = ESP_FAIL;

    switch(moduleState_ens)
    {
        case STATE_INITIALIZED:
        case STATE_DISCONNECTED:
            if(NULL != client_xps)
            {
                result_st = esp_mqtt_client_start(client_xps);
                moduleState_ens = STATE_CONNECT_IN_PROGRESS;
            }
            else
            {
                result_st = ESP_FAIL;
            }
            break;
        case STATE_CONNECTED:
        case STATE_CONNECT_IN_PROGRESS:
            result_st = ESP_OK;
            break;
        default:
            result_st = ESP_FAIL;
            break;
    }

    return(result_st);
}

/**---------------------------------------------------------------------------------------
 * @brief     Initialize subscription parameter
*//*-----------------------------------------------------------------------------------*/
esp_err_t myMqtt_InitSubParam(myMqtt_substParam_t *subsParam_stp)
{
    esp_err_t result_st = ESP_FAIL;

    memset(subsParam_stp, 0U, sizeof(myMqtt_substParam_t));

    return(result_st);
}

/**---------------------------------------------------------------------------------------
 * @brief     Allocate subscribe handle
*//*-----------------------------------------------------------------------------------*/
mqtt_subsHdl_t myMqtt_AllocSub_xp(myMqtt_substParam_t *subsParam_stp)
{
    mqtt_subsHdl_t handle_xp;

    if(NULL != subsParam_stp)
    {
        handle_xp = malloc(sizeof(myMqtt_obj_t));
        if(NULL != handle_xp)
        {
            // initialize the handle data
            memcpy(&handle_xp->param_st, subsParam_stp, sizeof(handle_xp->param_st));
            handle_xp->subscribed_bol = false;

            // add the handle to the list, either as first element or at the end
            AddSubsToList_vd(handle_xp);
        }
    }
    else
    {
        handle_xp = NULL;
    }

    return(handle_xp);
}

/**---------------------------------------------------------------------------------------
 * @brief     Deallocate subscribe handle
*//*-----------------------------------------------------------------------------------*/
esp_err_t myMqtt_DeAllocSub_st(mqtt_subsHdl_t subsHdl_xp)
{
    esp_err_t result_st = ESP_FAIL;

    if(NULL != subsHdl_xp)
    {
        if(true == subsHdl_xp->subscribed_bol)
        {
            // topic is still subscribed, first remove the subscription
            result_st = myMqtt_UnSubscribe(subsHdl_xp);

            // now, remove subscription entry from list
            RemoveSubsFromList_vd(subsHdl_xp);
        }
    }

    return(result_st);
}

/**---------------------------------------------------------------------------------------
 * @brief     Subscribe to a MQTT message
*//*-----------------------------------------------------------------------------------*/
esp_err_t myMqtt_Subscribe_xp(mqtt_subsHdl_t subsHdl_xp)
{
    esp_err_t result_st = ESP_FAIL;

    if(NULL != subsHdl_xp)
    {
        result_st = esp_mqtt_client_subscribe(client_xps,
                                                (char *)&subsHdl_xp->param_st.topic_u8a[0],
                                                subsHdl_xp->param_st.qos_u32);
        if(ESP_OK == result_st)
        {
            subsHdl_xp->subscribed_bol = true;
        }
    }
    else
    {
        result_st = ESP_FAIL;
    }
    return(result_st);
}

/**---------------------------------------------------------------------------------------
 * @brief     Un-Subscribe MQTT message
 * @author    S. Wink
 * @date      25. Mar. 2019
 * @param     subsHdl_xp         subscription handle
 * @return    ESP_OK if the message was successful un-subscribed
*//*-----------------------------------------------------------------------------------*/
esp_err_t myMqtt_UnSubscribe(mqtt_subsHdl_t subsHdl_xp)
{
    esp_err_t result_st = ESP_FAIL;

    if(NULL != subsHdl_xp)
    {
        result_st = esp_mqtt_client_unsubscribe(client_xps,
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

    return(result_st);
}

/**---------------------------------------------------------------------------------------
 * @brief     Disconnects from the MQTT broker
 * @author    S. Wink
 * @date      25. Mar. 2019
 * @return    n/a
*//*-----------------------------------------------------------------------------------*/
esp_err_t myMqtt_Disconnect(void)
{
    esp_err_t result_st = ESP_FAIL;

    return(result_st);
}

/****************************************************************************************/
/* Local functions: */

/**---------------------------------------------------------------------------------------
 * @brief     checks if the list is empty
 * @author    S. Wink
 * @date      24. Jan. 2019
 * @return    true if empty, else false
*//*------------------------------------------------------------------------------------*/
static bool IsListEmpty_bol(void)
{
    bool retVal_bol = false;

    if(NULL == list_xps)
    {
        retVal_bol = true;
    }

    return(retVal_bol);
}

/**---------------------------------------------------------------------------------------
 * @brief     Add the subscription object handle to the list for later handling
 * @author    S. Wink
 * @date      24. Jan. 2019
 * @param     subsHdl_xp        subscription handler
 * @return    n/a
*//*------------------------------------------------------------------------------------*/
static void AddSubsToList_vd(mqtt_subsHdl_t subsHdl_xp)
{
    if(NULL == list_xps)
    {
        list_xps = subsHdl_xp;
        list_xps->next_xp = NULL;
    }
    else
    {
        mqtt_subsHdl_t current_xp = list_xps;
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
static void RemoveSubsFromList_vd(mqtt_subsHdl_t subsHdl_xp)
{
    mqtt_subsHdl_t last_xp = NULL;
    mqtt_subsHdl_t current_xp;

    if((NULL != subsHdl_xp) && (NULL != list_xps))
    {
        last_xp = list_xps;
        current_xp = list_xps->next_xp;

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
 * @brief     Handler for connection event
 * @author    S. Wink
 * @date      31. Mar. 2019
 * @param     event_stp        event structure
 * @return    n/a
*//*------------------------------------------------------------------------------------*/
static void HandleConnect_vd(esp_mqtt_event_handle_t event_stp)
{
    esp_err_t result_st = ESP_FAIL;

    if((STATE_DISCONNECTED == moduleState_ens) ||
       (STATE_CONNECT_IN_PROGRESS == moduleState_ens))
    {
        moduleState_ens = STATE_CONNECTED;
        ESP_LOGI(TAG, "mqtt connected...");

        // message to control task that MQTT is online

        // call all connect functions of registered users
        mqtt_subsHdl_t index_xps = list_xps;
        if(NULL != index_xps)
        {
            if(NULL != index_xps->param_st.conn_fp)
            {
                index_xps->param_st.conn_fp();
            }
            index_xps = index_xps->next_xp;
            while(NULL != index_xps)
            {
                if(NULL != index_xps->param_st.conn_fp)
                {
                    index_xps->param_st.conn_fp();
                }
                index_xps = index_xps->next_xp;
            }
        }

        result_st = esp_mqtt_client_subscribe(client_xps, "/topic/qos0", 0);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", result_st);

        result_st = esp_mqtt_client_subscribe(client_xps, "std/dev21/s/temp_hum/temp", 0);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", result_st);

        result_st = esp_mqtt_client_subscribe(client_xps, "/topic/qos1", 1);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", result_st);

        result_st = esp_mqtt_client_unsubscribe(client_xps, "/topic/qos1");
        ESP_LOGI(TAG, "sent unsubscribe successful, msg_id=%d", result_st);
    }
    else
    {
        ESP_LOGW(TAG, "unexpected mqtt state reached...");
        myMqtt_Disconnect();
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
    if((STATE_CONNECTED == moduleState_ens) ||
       (STATE_INITIALIZED == moduleState_ens))
    {
        moduleState_ens = STATE_DISCONNECTED;
        ESP_LOGI(TAG, "mqtt disconnected...");

        // message to control task that MQTT is offline

        // call all registered callback functions for disconnection event
        // call all connect functions of registered users
        mqtt_subsHdl_t index_xps = list_xps;
        if(NULL != index_xps)
        {
            if(NULL != index_xps->param_st.discon_fp)
            {
                index_xps->param_st.discon_fp();
            }
            index_xps = index_xps->next_xp;
            while(NULL != index_xps)
            {
                index_xps->param_st.discon_fp();
                index_xps = index_xps->next_xp;
            }
        }
    }
    else if(STATE_NOT_INITIALIZED == moduleState_ens)
    {
        ESP_LOGW(TAG, "disconnection before initialization...");
    }
    else
    {
        ESP_LOGW(TAG, "unexpected state reached while disconnection...");
    }
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
    esp_err_t result_st = ESP_FAIL;

    ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event_stp->msg_id);
    result_st = esp_mqtt_client_publish(client_xps, "/topic/qos0", "data", 0, 0, 0);
    ESP_LOGI(TAG, "sent publish successful, msg_id=%d", result_st);
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
    ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event_stp->msg_id);
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
    ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event_stp->msg_id);
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
    mqtt_subsHdl_t index_xps = list_xps;
    mqttif_msg_t msg_st;
    esp_err_t result_st;

    ESP_LOGI(TAG, "MQTT_EVENT_DATA");
    printf("TOPIC=%.*s\r\n", event_stp->topic_len, event_stp->topic);
    printf("DATA=%.*s\r\n", event_stp->data_len, event_stp->data);

    result_st = esp_mqtt_client_publish(client_xps, "std/dev99/r/light_one/toggle", "", 0, 0, 0);
    ESP_LOGI(TAG, "sent publish successful, msg_id=%d", result_st);


    msg_st.topic_chp = event_stp->topic;
    msg_st.topicLen_u32 = event_stp->topic_len;
    msg_st.dataLen_u32 = event_stp->data_len;
    msg_st.data_chp = event_stp->data;

    if(NULL != index_xps)
    {
        if(0U == strncmp(event_stp->topic,
                         (char *)&index_xps->param_st.topic_u8a[0],
                         event_stp->topic_len))
        {
            if(NULL != index_xps->param_st.dataRecv_fp)
            {
                index_xps->param_st.dataRecv_fp(&msg_st);
            }
        }
        index_xps = index_xps->next_xp;
        while(NULL != index_xps)
        {
            if(0U == strncmp(event_stp->topic,
                             (char *)&index_xps->param_st.topic_u8a[0],
                             event_stp->topic_len))
            {
                index_xps->param_st.dataRecv_fp(&msg_st);
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
    ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
}

/**---------------------------------------------------------------------------------------
 * @brief     Add the subscription object handle to the list for later handling
 * @author    S. Wink
 * @date      24. Jan. 2019
 * @param     subsHdl_xp        subscription handler
 * @return    n/a
*//*------------------------------------------------------------------------------------*/
static esp_err_t MqttEventHandler_st(esp_mqtt_event_handle_t event_stp)
{

    switch (event_stp->event_id)
    {
        case MQTT_EVENT_CONNECTED:
            HandleConnect_vd(event_stp);
            break;
        case MQTT_EVENT_DISCONNECTED:
            HandleDisconnect_vd(event_stp);
            break;
        case MQTT_EVENT_SUBSCRIBED:
            HandleSubscription_vd(event_stp);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            HandleUnsubscription_vd(event_stp);
            break;
        case MQTT_EVENT_PUBLISHED:
            HandlePublish_vd(event_stp);
            break;
        case MQTT_EVENT_DATA:
            HandleData_vd(event_stp);
            break;
        case MQTT_EVENT_ERROR:
            HandleError_vd(event_stp);
            break;
        default:
            HandleError_vd(event_stp);
            break;
    }
    return ESP_OK;
}



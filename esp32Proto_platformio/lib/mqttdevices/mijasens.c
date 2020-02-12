/****************************************************************************************
* FILENAME :        mijasens.c
*
* SHORT DESCRIPTION:
*   Source file for the mijasens module.  
*
* DETAILED DESCRIPTION :   N/A  
*
* AUTHOR :    Stephan Wink        CREATED ON :    01. Feb. 2020
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
#include "mijasens.h"

#include "stdbool.h"
#include "string.h"
#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"

#include "mqttif.h"
#include "paramif.h"

#include "bleDrv.h"
#include "mijaProcl.h"

#include "appIdent.h"
#include "utils.h"

/****************************************************************************************/
/* Local constant defines */

#define MAX_MIJA_SENSORS        5U
#define LOCATION_STRING_SIZE    20U

#define MQTT_SUBSCRIPTIONS_NUM  6U
#define MQTT_SUB_CMD_DEL_TABLE  "mija/delete"   // delete sensor table
#define MQTT_SUB_CMD_SAVE_TABLE "mija/para"     // saves the  
#define MQTT_SUB_BLE_PARA       "ble/para"      // r/w request for bluetooth parameter
#define MQTT_SUB_CMD_MIJA_PARA  "mija/para"     // r/w request for mija sensor para
#define MQTT_SUB_CMD_MIJA_DATA  "mija/data"     // read request for mija sensor 

#define MQTT_SUB_CMD_SEN_KNOW   "mija/know"     // set sensor to known
#define MQTT_SUB_CMD_SEN_LOC    "mija/loc"      // set the sensor location

#define MAX_PUB_WAIT            10000

#define MODULE_TAG              "mijasens"

#define TASK_STACK_SIZE         4096
#define TASK_PRIORITY           5
#define QUEUE_ELEMENTS          10


/****************************************************************************************/
/* Local function like makros */

#define CHECK_EXE(arg) utils_CheckAndLogExecution_bol(MODULE_TAG, arg, __LINE__)

/****************************************************************************************/
/* Local type definitions (enum, struct, union) */

typedef struct subsHandle_tag
{
    const char *subs_chp;
    mqttif_DataReceived_td OnReceiveCb_fcp;
}subsHandle_t;

typedef enum objectState_tag
{
    STATE_NOT_INITIALIZED,
    STATE_INITIALIZED,
    STATE_ACTIVE,
    STATE_DEACTIVATED,
}objectState_t;

typedef struct sensorParam_tag
{
    uint8_t macAddr_u8a[mija_SIZE_MAC_ADDR];
    char loc_cha[LOCATION_STRING_SIZE];
    uint8_t knownSens_u8;
}sensorParam_t;

typedef struct sensorObject_tag
{
    sensorParam_t para_st;
    mijaProcl_parsedData_t data_st;
}sensorObject_t;

typedef enum mqttState_tag
{
    MQTT_STATE_CONNECTED,
    MQTT_STATE_DISCONNECTED
}mqttState_t;

typedef struct scanParam_tag
{
    uint32_t scanDur_u32;
    uint32_t cycle_u32;
}scanParam_t;

typedef struct moduleData_tag
{
    mijasens_param_t param_st;
    objectState_t state_en;
    mqttState_t mqtt_en;
    mqttif_msg_t pubMsg_st;
    sensorObject_t sensors_sta[MAX_MIJA_SENSORS];
    char subs_chap[MQTT_SUBSCRIPTIONS_NUM][mqttif_MAX_SIZE_OF_TOPIC];
    uint16_t subsCounter_u16;
    bleDrv_param_t blePara_st;
    EventGroupHandle_t eventGroup_st;
    TaskHandle_t task_xp;
    QueueHandle_t queue_xp;
    paramif_objHdl_t scanParam_xp;
    paramif_objHdl_t sensTable_xp;
}objectData_t;

/****************************************************************************************/
/* Local functions prototypes: */
static esp_err_t LoadScanParameter_st(void); 
static esp_err_t LoadSensors_st(void);
static void OnConnectionHandler_vd(void);
static void OnDisconnectionHandler_vd(void);
static esp_err_t OnTableDeleteHandler_st(mqttif_msg_t *msg_stp);
static esp_err_t OnTableSaveHandler_st(mqttif_msg_t *msg_stp);
static esp_err_t OnMijaBleHandler_st(mqttif_msg_t *msg_stp);
static esp_err_t OnMijaDataRequestHandler_st(mqttif_msg_t *msg_stp);
static esp_err_t OnMijaKnownStatusSetHandler_st(mqttif_msg_t *msg_stp);
static esp_err_t OnMijaLocationSetHandler_st(mqttif_msg_t *msg_stp);

static void PublishSensorData_vd(uint8_t sensIdx_u8);
static void PublishSensorParam_vd(uint8_t sensIdx_u8);
static void PublishScanParameter_vd(void);

static void DriverCallback_vd(mijaProcl_parsedData_t *data_stp);
static void HandleQueueEvent_vd(void);
static void Task_vd(void *pvParameters);

/****************************************************************************************/
/* Local variables: */

static objectData_t this_sst;

static const int MQTT_CONNECT               = BIT0;
static const int MQTT_DISCONNECT            = BIT1;
static const int BLE_DATA_EVENT             = BIT2;

static const char *TAG                      = MODULE_TAG;

static const char *MQTT_PUB_SCAN            = "ble/scan";
static const char *MQTT_PUB_CYCL            = "ble/cycle";

static const char *MQTT_PUB_TEMP            = "mija/temp";
static const char *MQTT_PUB_HUM             = "mija/hum";
static const char *MQTT_PUB_BATT            = "mija/batt";
static const char *MQTT_PUB_MSGCNT          = "mija/cnt";
static const char *MQTT_PUB_ADDR            = "mija/addr";
static const char *MQTT_PUB_LOC             = "mija/loc";
static const char *MQTT_PUB_KNOW            = "mija/know";

const subsHandle_t subsHandle_csta[MQTT_SUBSCRIPTIONS_NUM] = 
{
    {MQTT_SUB_CMD_DEL_TABLE, OnTableDeleteHandler_st},
    {MQTT_SUB_CMD_SAVE_TABLE, OnTableSaveHandler_st},
    {MQTT_SUB_BLE_PARA, OnMijaBleHandler_st},
    {MQTT_SUB_CMD_MIJA_DATA, OnMijaDataRequestHandler_st},
    {MQTT_SUB_CMD_SEN_KNOW, OnMijaKnownStatusSetHandler_st},
    {MQTT_SUB_CMD_SEN_LOC, OnMijaLocationSetHandler_st},
};

static const char *SCAN_PARA_IDENT = "bleScan";
static const scanParam_t SCAN_DEFAULT_PARA = 
{
    .scanDur_u32 = 60,
    .cycle_u32 = 20,
};

static const char *SENSORS_PARA_IDENT = "mijaSen";

/****************************************************************************************/
/* Global functions (unlimited visibility) */

/**---------------------------------------------------------------------------------------
 * @brief     Initializes the initialization structure of the generic device module
 * @author    S. Wink
 * @date      01. Feb. 2020
*//*-----------------------------------------------------------------------------------*/
esp_err_t mijasens_InitializeParameter_st(mijasens_param_t *param_stp)
{
    esp_err_t result_st = ESP_FAIL;

    this_sst.state_en = STATE_NOT_INITIALIZED;
    this_sst.param_st.deviceName_chp = "dev99";
    this_sst.param_st.id_u8 = 1;
    this_sst.param_st.publishHandler_fp = NULL;

    if(NULL != param_stp)
    {
        param_stp->publishHandler_fp = NULL;
        param_stp->deviceName_chp = "dev99";
        param_stp->id_u8 = 0U;

        result_st = ESP_OK;
    }

    return(result_st);
}

/**---------------------------------------------------------------------------------------
 * @brief     Initialization of the generic device module
 * @author    S. Wink
 * @date      01. Feb. 2020
*//*-----------------------------------------------------------------------------------*/
esp_err_t mijasens_Initialize_st(mijasens_param_t *param_stp)
{
    esp_err_t result_st = ESP_FAIL;
    bool exeResult_bol = true;
    bleDrv_param_t params_st;


    esp_log_level_set(TAG, ESP_LOG_DEBUG);

    ESP_LOGD(TAG, "initialization started...");
    if(NULL != param_stp)
    {
        /* copy parameters and initialize internal module data */
        this_sst.param_st.deviceName_chp = param_stp->deviceName_chp;
        this_sst.param_st.id_u8 = param_stp->id_u8;
        this_sst.param_st.publishHandler_fp = param_stp->publishHandler_fp;
        this_sst.pubMsg_st.dataLen_u32 = 0;
        this_sst.pubMsg_st.topicLen_u32 = 0;
        this_sst.pubMsg_st.topic_chp = malloc(mqttif_MAX_SIZE_OF_TOPIC * sizeof(char));
        this_sst.pubMsg_st.data_chp = malloc(mqttif_MAX_SIZE_OF_DATA * sizeof(char));
        this_sst.pubMsg_st.qos_s32 = 1;
        this_sst.pubMsg_st.retain_s32 = 0;
        this_sst.mqtt_en = MQTT_STATE_DISCONNECTED;

        CHECK_EXE(LoadSensors_st());

        exeResult_bol &= CHECK_EXE(LoadScanParameter_st());
        exeResult_bol &= CHECK_EXE(bleDrv_InitializeParameter_st(&params_st));
	    params_st.cycleTimeInSec_u32 = this_sst.blePara_st.cycleTimeInSec_u32;
	    params_st.scanDurationInSec_u32 = this_sst.blePara_st.scanDurationInSec_u32;
	    params_st.dataCb_fp = DriverCallback_vd;
	    exeResult_bol &= CHECK_EXE(bleDrv_Initialize_st(&params_st));

        this_sst.eventGroup_st = xEventGroupCreate();
        exeResult_bol &= (NULL != this_sst.eventGroup_st);

        xTaskCreate(Task_vd, "mijasensTask", TASK_STACK_SIZE, NULL, 
                                        TASK_PRIORITY, &this_sst.task_xp);
        exeResult_bol &= (NULL != this_sst.task_xp);

        this_sst.queue_xp = xQueueCreate(QUEUE_ELEMENTS, sizeof(mijaProcl_parsedData_t));
        exeResult_bol &= (NULL != this_sst.queue_xp);
    }
    else
    {
        exeResult_bol = false;
    }

    if(true == exeResult_bol)
    {
        this_sst.state_en = STATE_INITIALIZED;

        result_st = ESP_OK;
    }

    ESP_LOGD(TAG, "initialization completed...");
    return(result_st);
}

/**---------------------------------------------------------------------------------------
 * @brief     Get subscriptions from the generic device by index
 * @author    S. Wink
 * @date      01. Feb. 2020
*//*-----------------------------------------------------------------------------------*/
extern bool mijasens_GetSubscriptionByIndex_bol(uint16_t idx_u16,
                                                mqttif_substParam_t *dest_stp)
{
    bool indexIsValid_bol = false;

    char tempSubs_ca[mqttif_MAX_SIZE_OF_TOPIC];

    if(idx_u16 < MQTT_SUBSCRIPTIONS_NUM)
    {
        indexIsValid_bol = true;
        dest_stp->conn_fp = OnConnectionHandler_vd;
        dest_stp->discon_fp = OnDisconnectionHandler_vd;
        dest_stp->dataRecv_fp = subsHandle_csta[idx_u16].OnReceiveCb_fcp;
        dest_stp->qos_u32 = 0;

        memset(&dest_stp->topic_u8a[0], 0U, sizeof(dest_stp->topic_u8a));
        utils_BuildReceiveTopic_chp(this_sst.param_st.deviceName_chp, 
                                this_sst.param_st.id_u8,
                                subsHandle_csta[idx_u16].subs_chp,
                                &tempSubs_ca[0]);
        memcpy(&dest_stp->topic_u8a[0], &tempSubs_ca[0],
             utils_MIN(strlen(tempSubs_ca), mqttif_MAX_SIZE_OF_TOPIC));
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
 * @date      01. Feb. 2020
*//*-----------------------------------------------------------------------------------*/
esp_err_t mijasens_Activate_st(void)
{
    esp_err_t exeResult_st = ESP_FAIL;

    if(    (STATE_INITIALIZED == this_sst.state_en) 
        || (STATE_DEACTIVATED == this_sst.state_en))
    {
        this_sst.state_en = STATE_ACTIVE;
        bleDrv_Activate_st();
        exeResult_st = ESP_OK;
    }

    return(exeResult_st);
}

/**---------------------------------------------------------------------------------------
 * @brief     Deacrivates the generic device function
 * @author    S. Wink
 * @date      01. Feb. 2020
*//*-----------------------------------------------------------------------------------*/
esp_err_t mijasens_Deactivate_st(void)
{
    esp_err_t exeResult_st = ESP_FAIL;

    if(    (STATE_INITIALIZED == this_sst.state_en) 
        || (STATE_ACTIVE == this_sst.state_en))
    {
        this_sst.state_en = STATE_DEACTIVATED;
        bleDrv_Deactivate_st();
        exeResult_st = ESP_OK;
    }

    return(exeResult_st);
}

/****************************************************************************************/
/* Local functions: */

/**--------------------------------------------------------------------------------------
 * @brief     Load the scan parameter from the nvmem
 * @author    S. Wink
 * @date      01. Feb. 2020
 * @return    ESP_OK if successful, else ESP_FAIL
*//*-----------------------------------------------------------------------------------*/
static esp_err_t LoadScanParameter_st(void)
{
    bool exeResult_bol = true;
    esp_err_t result_st = ESP_OK;
    paramif_allocParam_t deviceAllocParam_st;
    scanParam_t para_st;


    exeResult_bol &= CHECK_EXE(paramif_InitializeAllocParameter_td(
                                                &deviceAllocParam_st));
    deviceAllocParam_st.length_u16 = sizeof(scanParam_t);
    deviceAllocParam_st.defaults_u8p = (uint8_t *)&SCAN_DEFAULT_PARA;
    deviceAllocParam_st.nvsIdent_cp = SCAN_PARA_IDENT;
    this_sst.scanParam_xp = paramif_Allocate_stp(&deviceAllocParam_st);
    paramif_PrintHandle_vd(this_sst.scanParam_xp );
    exeResult_bol &= CHECK_EXE(paramif_Read_td(this_sst.scanParam_xp, 
                                                (uint8_t *) &para_st));
    this_sst.blePara_st.cycleTimeInSec_u32 = para_st.cycle_u32;
	this_sst.blePara_st.scanDurationInSec_u32 = para_st.scanDur_u32;
    
    if(false == exeResult_bol)
    {
        result_st = ESP_FAIL;
    }
    return(result_st);

}

/**--------------------------------------------------------------------------------------
 * @brief     Load the sknown sensors from the nvmem
 * @author    S. Wink
 * @date      01. Feb. 2020
 * @return    ESP_OK if successful, else ESP_FAIL
*//*-----------------------------------------------------------------------------------*/
static esp_err_t LoadSensors_st(void)
{
    bool exeResult_bol = true;
    esp_err_t result_st = ESP_OK;
    paramif_allocParam_t deviceAllocParam_st;
    sensorParam_t localPara_sta[MAX_MIJA_SENSORS];

    memset(&localPara_sta[0], 0U, sizeof(localPara_sta));
    exeResult_bol &= CHECK_EXE(paramif_InitializeAllocParameter_td(
                                                &deviceAllocParam_st));
    deviceAllocParam_st.length_u16 = sizeof(localPara_sta);
    deviceAllocParam_st.defaults_u8p = NULL;
    deviceAllocParam_st.nvsIdent_cp = SENSORS_PARA_IDENT;
    this_sst.sensTable_xp = paramif_Allocate_stp(&deviceAllocParam_st);
    paramif_PrintHandle_vd(this_sst.sensTable_xp);
    exeResult_bol &= CHECK_EXE(paramif_Read_td(this_sst.sensTable_xp, 
                                                (uint8_t *) &localPara_sta[0]));
    
    for(uint8_t idx_u8 = 0U; idx_u8 <  MAX_MIJA_SENSORS; idx_u8++)
    {
        memcpy(&this_sst.sensors_sta[idx_u8].para_st, &localPara_sta[idx_u8], 
                sizeof(sensorParam_t));
    }
    
    if(true == exeResult_bol)
    {
        result_st = ESP_FAIL;
    }
    return(result_st);

}

/**---------------------------------------------------------------------------------------
 * @brief     Handler when connected to mqtt broker
 * @author    S. Wink
 * @date      01. Feb. 2020
 * @return    n/a
*//*-----------------------------------------------------------------------------------*/
static void OnConnectionHandler_vd(void)
{
    this_sst.mqtt_en = MQTT_STATE_CONNECTED;
    xEventGroupSetBits(this_sst.eventGroup_st, MQTT_CONNECT);
}

/**---------------------------------------------------------------------------------------
 * @brief     Handler when disconnected from mqtt broker
 * @author    S. Wink
 * @date      01. Feb. 2020
 * @return    n/a
*//*-----------------------------------------------------------------------------------*/
static void OnDisconnectionHandler_vd(void)
{
    this_sst.mqtt_en = MQTT_STATE_DISCONNECTED;
    xEventGroupSetBits(this_sst.eventGroup_st, MQTT_DISCONNECT);
}

/**--------------------------------------------------------------------------------------
 * @brief     Handler when table delete subscription recieved
 * @author    S. Wink
 * @date      01. Feb. 2020
 * @param     msg_stp   message data pointer
 * @return    ESP_OK if successful, else ESP_FAIL
*//*-----------------------------------------------------------------------------------*/
static esp_err_t OnTableDeleteHandler_st(mqttif_msg_t *msg_stp)
{
    esp_err_t result_st = ESP_OK;

    ESP_LOGD(TAG, "message topic:%.*s received with data:%.*s",
                    msg_stp->topicLen_u32, msg_stp->topic_chp,
                    msg_stp->dataLen_u32, msg_stp->data_chp);
    
    memset(this_sst.sensors_sta, 0U, sizeof(this_sst.sensors_sta));

    return(result_st);
}

/**--------------------------------------------------------------------------------------
 * @brief     Handler when table save subscription recieved
 * @author    S. Wink
 * @date      01. Feb. 2020
 * @param     msg_stp   message data pointer
 * @return    ESP_OK if successful, else ESP_FAIL
*//*-----------------------------------------------------------------------------------*/
static esp_err_t OnTableSaveHandler_st(mqttif_msg_t *msg_stp)
{
    esp_err_t result_st = ESP_OK;

    ESP_LOGD(TAG, "message topic:%.*s received with data:%.*s",
                    msg_stp->topicLen_u32, msg_stp->topic_chp,
                    msg_stp->dataLen_u32, msg_stp->data_chp);
    
    ESP_LOGD(TAG, "unimplemented feature to save table");

    return(result_st);
}

/**--------------------------------------------------------------------------------------
 * @brief     Handler to request read/write the scan and cycle parameter
 * @author    S. Wink
 * @date      01. Feb. 2020
 * @param     msg_stp   message data pointer
 * @return    ESP_OK if successful, else ESP_FAIL
*//*-----------------------------------------------------------------------------------*/
static esp_err_t OnMijaBleHandler_st(mqttif_msg_t *msg_stp)
{
    esp_err_t result_st = ESP_OK;
    bool exeResult_bol = true;

    ESP_LOGD(TAG, "message topic:%.*s received with data:%.*s",
                    msg_stp->topicLen_u32, msg_stp->topic_chp,
                    msg_stp->dataLen_u32, msg_stp->data_chp);
    
    if(0 == msg_stp->dataLen_u32)
    {
        PublishScanParameter_vd();
    }
    else
    {
        char *duration_chp = strtok(msg_stp->data_chp, " ");
        char *cycle_chp = strtok(msg_stp->data_chp, " ");
        this_sst.blePara_st.cycleTimeInSec_u32 = atoi(cycle_chp);
        this_sst.blePara_st.scanDurationInSec_u32 = atoi(duration_chp);
        exeResult_bol = true;
    }

    if(false == exeResult_bol)
    {
        result_st = ESP_FAIL;
    }

    return(result_st);
}

/**--------------------------------------------------------------------------------------
 * @brief     Handler to request mija measurement data
 * @author    S. Wink
 * @date      01. Feb. 2020
 * @param     msg_stp   message data pointer
 * @return    ESP_OK if successful, else ESP_FAIL
*//*-----------------------------------------------------------------------------------*/
static esp_err_t OnMijaDataRequestHandler_st(mqttif_msg_t *msg_stp)
{
    esp_err_t result_st = ESP_OK;

    ESP_LOGD(TAG, "message topic:%.*s received with data:%.*s",
                    msg_stp->topicLen_u32, msg_stp->topic_chp,
                    msg_stp->dataLen_u32, msg_stp->data_chp);
    
    ESP_LOGD(TAG, "unimplemented feature to request mija measurement data");

    return(result_st);
}

/**--------------------------------------------------------------------------------------
 * @brief     Handler to set mija known status parameter
 * @author    S. Wink
 * @date      01. Feb. 2020
 * @param     msg_stp   message data pointer
 * @return    ESP_OK if successful, else ESP_FAIL
*//*-----------------------------------------------------------------------------------*/
static esp_err_t OnMijaKnownStatusSetHandler_st(mqttif_msg_t *msg_stp)
{
    esp_err_t result_st = ESP_OK;

    ESP_LOGD(TAG, "message topic:%.*s received with data:%.*s",
                    msg_stp->topicLen_u32, msg_stp->topic_chp,
                    msg_stp->dataLen_u32, msg_stp->data_chp);
    if(1U == msg_stp->dataLen_u32)
    {
        char *senId_chp = strtok(msg_stp->data_chp, " ");
        if(MAX_MIJA_SENSORS > atoi(senId_chp))
        {
            PublishSensorParam_vd(atoi(senId_chp)); 
        }   
        else
        {
            result_st = ESP_FAIL;
        }
             
    }
    else if( 1U < msg_stp->dataLen_u32)
    {
        char *senId_chp = strtok(msg_stp->data_chp, " ");
        char *state_chp = strtok(msg_stp->data_chp, " "); 

        if((MAX_MIJA_SENSORS > atoi(senId_chp)) 
            && ((0U == atoi(state_chp) || (1U == atoi(state_chp)))))
        {
            this_sst.sensors_sta[atoi(senId_chp)].para_st.knownSens_u8 = atoi(state_chp);
            PublishSensorParam_vd(atoi(senId_chp));
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

/**--------------------------------------------------------------------------------------
 * @brief     Handler to set mija location parameter
 * @author    S. Wink
 * @date      01. Feb. 2020
 * @param     msg_stp   message data pointer
 * @return    ESP_OK if successful, else ESP_FAIL
*//*-----------------------------------------------------------------------------------*/
static esp_err_t OnMijaLocationSetHandler_st(mqttif_msg_t *msg_stp)
{
    esp_err_t result_st = ESP_OK;

    ESP_LOGD(TAG, "message topic:%.*s received with data:%.*s",
                    msg_stp->topicLen_u32, msg_stp->topic_chp,
                    msg_stp->dataLen_u32, msg_stp->data_chp);
    
    ESP_LOGD(TAG, "unimplemented feature to set the mija sensor location parameter");

    return(result_st);
}

/**---------------------------------------------------------------------------------------
 * @brief     function to send sensor data
 * @author    S. Wink
 * @date      01. Feb. 2020
 * @return    n/a
*//*-----------------------------------------------------------------------------------*/
static void PublishSensorData_vd(uint8_t sensIdx_u8)
{
    if(MQTT_STATE_CONNECTED == this_sst.mqtt_en)
    {
        utils_BuildSendTopic_chp(this_sst.param_st.deviceName_chp, 
                                    this_sst.param_st.id_u8 + sensIdx_u8,
                                    MQTT_PUB_TEMP, this_sst.pubMsg_st.topic_chp);
        this_sst.pubMsg_st.topicLen_u32 = strlen(this_sst.pubMsg_st.topic_chp);
        this_sst.pubMsg_st.dataLen_u32 = sprintf(this_sst.pubMsg_st.data_chp, "%.2f",
                                    this_sst.sensors_sta[sensIdx_u8].data_st.temperature_f32);
        CHECK_EXE(this_sst.param_st.publishHandler_fp(&this_sst.pubMsg_st, MAX_PUB_WAIT));
        ESP_LOGD(TAG, "publish: %s :: %s", this_sst.pubMsg_st.topic_chp, 
                    this_sst.pubMsg_st.data_chp);
        
        utils_BuildSendTopic_chp(this_sst.param_st.deviceName_chp, 
                                    this_sst.param_st.id_u8 + sensIdx_u8,
                                    MQTT_PUB_HUM, this_sst.pubMsg_st.topic_chp);
        this_sst.pubMsg_st.topicLen_u32 = strlen(this_sst.pubMsg_st.topic_chp);
        this_sst.pubMsg_st.dataLen_u32 = sprintf(this_sst.pubMsg_st.data_chp, "%.2f",
                                    this_sst.sensors_sta[sensIdx_u8].data_st.humidity_f32);
        CHECK_EXE(this_sst.param_st.publishHandler_fp(&this_sst.pubMsg_st, MAX_PUB_WAIT));
        ESP_LOGD(TAG, "publish: %s :: %s", this_sst.pubMsg_st.topic_chp, 
                    this_sst.pubMsg_st.data_chp);
        
        utils_BuildSendTopic_chp(this_sst.param_st.deviceName_chp, 
                                    this_sst.param_st.id_u8 + sensIdx_u8,
                                    MQTT_PUB_BATT, this_sst.pubMsg_st.topic_chp);
        this_sst.pubMsg_st.topicLen_u32 = strlen(this_sst.pubMsg_st.topic_chp);
        this_sst.pubMsg_st.dataLen_u32 = sprintf(this_sst.pubMsg_st.data_chp, "%.2f",
                                    this_sst.sensors_sta[sensIdx_u8].data_st.battery_f32);
        CHECK_EXE(this_sst.param_st.publishHandler_fp(&this_sst.pubMsg_st, MAX_PUB_WAIT));
        ESP_LOGD(TAG, "publish: %s :: %s", this_sst.pubMsg_st.topic_chp, 
                    this_sst.pubMsg_st.data_chp);

        utils_BuildSendTopic_chp(this_sst.param_st.deviceName_chp, 
                                    this_sst.param_st.id_u8 + sensIdx_u8,
                                    MQTT_PUB_MSGCNT, this_sst.pubMsg_st.topic_chp);
        this_sst.pubMsg_st.topicLen_u32 = strlen(this_sst.pubMsg_st.topic_chp);
        this_sst.pubMsg_st.dataLen_u32 = sprintf(this_sst.pubMsg_st.data_chp, "%d",
                                    this_sst.sensors_sta[sensIdx_u8].data_st.msgCnt_u8);
        CHECK_EXE(this_sst.param_st.publishHandler_fp(&this_sst.pubMsg_st, MAX_PUB_WAIT));
        ESP_LOGD(TAG, "publish: %s :: %s", this_sst.pubMsg_st.topic_chp, 
                    this_sst.pubMsg_st.data_chp);
    }               
}

/**---------------------------------------------------------------------------------------
 * @brief     function to send sensor data
 * @author    S. Wink
 * @date      01. Feb. 2020
 * @return    n/a
*//*-----------------------------------------------------------------------------------*/
static void PublishSensorParam_vd(uint8_t sensIdx_u8)
{


    if(MQTT_STATE_CONNECTED == this_sst.mqtt_en)
    {
        utils_BuildSendTopic_chp(this_sst.param_st.deviceName_chp, 
                                    this_sst.param_st.id_u8 + sensIdx_u8 + sensIdx_u8,
                                    MQTT_PUB_ADDR, this_sst.pubMsg_st.topic_chp);
        this_sst.pubMsg_st.topicLen_u32 = strlen(this_sst.pubMsg_st.topic_chp);
        this_sst.pubMsg_st.dataLen_u32 = 
                    sprintf(this_sst.pubMsg_st.data_chp, "%02X:%02X:%02X:%02X:%02X:%02X",
                                this_sst.sensors_sta[sensIdx_u8].para_st.macAddr_u8a[0],
                                this_sst.sensors_sta[sensIdx_u8].para_st.macAddr_u8a[1],
                                this_sst.sensors_sta[sensIdx_u8].para_st.macAddr_u8a[2],
                                this_sst.sensors_sta[sensIdx_u8].para_st.macAddr_u8a[3],
                                this_sst.sensors_sta[sensIdx_u8].para_st.macAddr_u8a[4],
                                this_sst.sensors_sta[sensIdx_u8].para_st.macAddr_u8a[5]);
        CHECK_EXE(this_sst.param_st.publishHandler_fp(&this_sst.pubMsg_st, MAX_PUB_WAIT));
        ESP_LOGD(TAG, "publish: %s :: %s", this_sst.pubMsg_st.topic_chp, 
                    this_sst.pubMsg_st.data_chp);
        
        utils_BuildSendTopic_chp(this_sst.param_st.deviceName_chp, 
                                    this_sst.param_st.id_u8 + sensIdx_u8,
                                    MQTT_PUB_LOC, this_sst.pubMsg_st.topic_chp);
        this_sst.pubMsg_st.topicLen_u32 = strlen(this_sst.pubMsg_st.topic_chp);
        this_sst.pubMsg_st.dataLen_u32 = sprintf(this_sst.pubMsg_st.data_chp, "%s",
                                    this_sst.sensors_sta[sensIdx_u8].para_st.loc_cha);
        CHECK_EXE(this_sst.param_st.publishHandler_fp(&this_sst.pubMsg_st, MAX_PUB_WAIT));
        ESP_LOGD(TAG, "publish: %s :: %s", this_sst.pubMsg_st.topic_chp, 
                    this_sst.pubMsg_st.data_chp);
        
        utils_BuildSendTopic_chp(this_sst.param_st.deviceName_chp, 
                                    this_sst.param_st.id_u8 + sensIdx_u8,
                                    MQTT_PUB_KNOW, this_sst.pubMsg_st.topic_chp);
        this_sst.pubMsg_st.topicLen_u32 = strlen(this_sst.pubMsg_st.topic_chp);
        this_sst.pubMsg_st.dataLen_u32 = sprintf(this_sst.pubMsg_st.data_chp, "%d",
                                    this_sst.sensors_sta[sensIdx_u8].para_st.knownSens_u8);
        CHECK_EXE(this_sst.param_st.publishHandler_fp(&this_sst.pubMsg_st, MAX_PUB_WAIT));
        ESP_LOGD(TAG, "publish: %s :: %s", this_sst.pubMsg_st.topic_chp, 
                    this_sst.pubMsg_st.data_chp);
    }               
}

/**---------------------------------------------------------------------------------------
 * @brief     function to publish scan parameter
 * @author    S. Wink
 * @date      01. Feb. 2020
 * @return    n/a
*//*-----------------------------------------------------------------------------------*/
static void PublishScanParameter_vd(void)
{

    if(MQTT_STATE_CONNECTED == this_sst.mqtt_en)
    {
        utils_BuildSendTopic_chp(this_sst.param_st.deviceName_chp, this_sst.param_st.id_u8,
                                    MQTT_PUB_SCAN, this_sst.pubMsg_st.topic_chp);
        this_sst.pubMsg_st.topicLen_u32 = strlen(this_sst.pubMsg_st.topic_chp);
        this_sst.pubMsg_st.dataLen_u32 = sprintf(this_sst.pubMsg_st.data_chp, "%d",
                                    this_sst.blePara_st.scanDurationInSec_u32);
        CHECK_EXE(this_sst.param_st.publishHandler_fp(&this_sst.pubMsg_st, MAX_PUB_WAIT));
        ESP_LOGD(TAG, "publish: %s :: %s", this_sst.pubMsg_st.topic_chp, 
                    this_sst.pubMsg_st.data_chp);

        utils_BuildSendTopic_chp(this_sst.param_st.deviceName_chp, this_sst.param_st.id_u8,
                                    MQTT_PUB_CYCL, this_sst.pubMsg_st.topic_chp);
        this_sst.pubMsg_st.topicLen_u32 = strlen(this_sst.pubMsg_st.topic_chp);
        this_sst.pubMsg_st.dataLen_u32 = sprintf(this_sst.pubMsg_st.data_chp, "%d",
                                    this_sst.blePara_st.scanDurationInSec_u32);
        CHECK_EXE(this_sst.param_st.publishHandler_fp(&this_sst.pubMsg_st, MAX_PUB_WAIT));
        ESP_LOGD(TAG, "publish: %s :: %s", this_sst.pubMsg_st.topic_chp, 
                    this_sst.pubMsg_st.data_chp);
    }               
}

/**--------------------------------------------------------------------------------------
 * @brief     Timer callback to re-trigger the scan process
 * @author    S. Wink
 * @date      17. Jan. 2020
 * @param     xTimer_xp     handle of the timer
 * @return    n/a
*//*-----------------------------------------------------------------------------------*/
static void DriverCallback_vd(mijaProcl_parsedData_t *data_stp)
{
	if((NULL != data_stp) && (NULL != this_sst.queue_xp))
    {
        if(pdPASS == xQueueSendToBack(this_sst.queue_xp, (void *) data_stp,
                                            (TickType_t) 10))
        {
            // notify task that new data is available in the queue
            if(NULL != this_sst.eventGroup_st)
            {
                xEventGroupSetBits(this_sst.eventGroup_st, BLE_DATA_EVENT);
            }
        }
    }
}

/**--------------------------------------------------------------------------------------
 * @brief     Handle the queue receive event and print data
 * @author    S. Wink
 * @date      01. Feb. 2020
 * @param     xTimer_xp     handle of the timer
 * @return    n/a
*//*-----------------------------------------------------------------------------------*/
static void HandleQueueEvent_vd(void)
{
    mijaProcl_parsedData_t data_st;

    if(NULL != this_sst.queue_xp)
    {
        // Receive a message on the created queue.  Block for 10 ticks if a
        // message is not immediately available.
        while(pdTRUE == xQueueReceive(this_sst.queue_xp, &data_st, (TickType_t) 10))
        {
            //mijaProcl_PrintMessage_bol(&data_st);

            // search if the sensor is already allocated
            uint8_t sensIdFound_u8 = MAX_MIJA_SENSORS;
            uint8_t idx_u8 = 0U;

            while(idx_u8 < MAX_MIJA_SENSORS)
            {
                if(0 == memcmp(&this_sst.sensors_sta[idx_u8].para_st.macAddr_u8a, 
                                    data_st.macAddr_u8a, sizeof(data_st.macAddr_u8a)))
                {
                    sensIdFound_u8 = idx_u8;
                    mijaProcl_SetData_bol(&this_sst.sensors_sta[sensIdFound_u8].data_st, 
                                            &data_st);
                    break;
                }
                idx_u8++;
            }

            if(sensIdFound_u8 == MAX_MIJA_SENSORS)
            {
                idx_u8 = 0U;
                // search for an empty sensor list element
                while(idx_u8 < MAX_MIJA_SENSORS)
                {
                    uint8_t default_u8a[mija_SIZE_MAC_ADDR];
                    memset(&default_u8a[0], 0U, sizeof(default_u8a));
                    if(0 == memcmp(&this_sst.sensors_sta[idx_u8].para_st.macAddr_u8a, 
                                    default_u8a, sizeof(default_u8a)))
                    {
                        sensIdFound_u8 = idx_u8;
                        // first: copy the complete data set
                        memcpy(&this_sst.sensors_sta[idx_u8].data_st, &data_st, 
                                sizeof(data_st));
                        // second: set the mac address in the parameter section for later 
                        // identification
                        memcpy(&this_sst.sensors_sta[idx_u8].para_st.macAddr_u8a[0], 
                                    &data_st.macAddr_u8a[0], 
                                    sizeof(data_st.macAddr_u8a));
                        break;
                    }
                    idx_u8++;
                }
            }

            if(sensIdFound_u8 < MAX_MIJA_SENSORS)
            {
                
                PublishSensorData_vd(sensIdFound_u8);
                PublishSensorParam_vd(sensIdFound_u8);
                ESP_LOGD(TAG, "*** Sensor ID: %d ***", sensIdFound_u8);
                mijaProcl_PrintMessage_bol(&this_sst.sensors_sta[sensIdFound_u8].data_st);
            }
        }
    }
}

/**---------------------------------------------------------------------------------------
 * @brief     task routine for the mqtt handling
 * @author    S. Wink
 * @date      01. Feb. 2020
 * @param     pvParameters      interface variable from freertos
*//*-----------------------------------------------------------------------------------*/
static void Task_vd(void *pvParameters)
{
    EventBits_t uxBits_st;
    uint32_t bits_u32 =   MQTT_CONNECT | MQTT_DISCONNECT | BLE_DATA_EVENT;

    ESP_LOGD(TAG, "mijasens-task started...");
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
        if(0 != (uxBits_st & BLE_DATA_EVENT))
        {
			HandleQueueEvent_vd();
        }
    }
}

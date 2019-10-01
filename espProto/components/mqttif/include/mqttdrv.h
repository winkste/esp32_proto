/*****************************************************************************************
* FILENAME :        myMqtt.h
*
* DESCRIPTION :
*       Header file for
*
* Date: 24. January 2019
*
* NOTES :
*
* Copyright (c) [2019] [Stephan Wink]
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
#ifndef MQTTDRV_H
#define MQTTDRV_H
/****************************************************************************************/
/* Imported header files: */

#include "mqttif.h"
#include "stdint.h"
#include "esp_err.h"

#include "stdbool.h"


/****************************************************************************************/
/* Global constant defines: */
#define mqttdrv_MAX_HOST_NAME_LEN    64U
#define mqttdrv_MAX_USER_NAME_LEN    20U
#define mqttdrv_MAX_USER_PWD_LEN     20U

/****************************************************************************************/
/* Global function like macro defines (to be avoided): */

/****************************************************************************************/
/* Global type definitions (enum (en), struct (st), union (un), typedef (tx): */
typedef struct mqttdrv_param_tag
{
    uint8_t host_u8a[mqttdrv_MAX_HOST_NAME_LEN];     /*!< MQTT server domain (ipv4 as string) */
    uint32_t port_u32;                              /*!< MQTT server port */
    uint8_t userName_u8a[mqttdrv_MAX_USER_NAME_LEN]; /*!< MQTT username */
    uint8_t userPwd_u8a[mqttdrv_MAX_USER_PWD_LEN];   /*!< MQTT password */
}mqttdrv_param_t;

typedef struct mqttdrv_substParam_tag
{
    uint8_t topic_u8a[mqttif_MAX_SIZE_OF_TOPIC];
    uint32_t qos_u32;
    mqttif_Connected_td conn_fp;
    mqttif_Disconnected_td discon_fp;
    mqttif_DataReceived_td dataRecv_fp;
}mqttdrv_substParam_t;

typedef struct mqttdrv_subsObj_tag* mqttdrv_subsHdl_t;
/****************************************************************************************/
/* Global function definitions: */

/**---------------------------------------------------------------------------------------
 * @brief     Initializes the initialization structure of the mqtt module
 * @author    S. Wink
 * @date      25. Mar. 2019
 * @param     param_stp         pointer to the configuration structure
 * @return    n/a
*//*-----------------------------------------------------------------------------------*/
extern esp_err_t mqttdrv_InitializeParameter(mqttdrv_param_t *param_stp);

/**---------------------------------------------------------------------------------------
 * @brief     Initialization of the mqtt module
 * @author    S. Wink
 * @date      25. Mar. 2019
 * @param     param_stp         pointer to the configuration structure
 * @return    n/a
*//*-----------------------------------------------------------------------------------*/
extern esp_err_t mqttdrv_Initialize(mqttdrv_param_t *param_stp);

/**---------------------------------------------------------------------------------------
 * @brief     Starts the MQTT demon if it was initialized
 * @author    S. Wink
 * @date      25. Mar. 2019
 * @return    n/a
*//*-----------------------------------------------------------------------------------*/
extern void mqttdrv_StartMqttDemon(void);

/**---------------------------------------------------------------------------------------
 * @brief     Initialize subscription parameter
 * @author    S. Wink
 * @date      25. Mar. 2019
 * @param     subsParam_stp         user structure for mqtt subs/pubs
 * @return    n/a
*//*-----------------------------------------------------------------------------------*/
extern esp_err_t mqttdrv_InitSubscriptParam(mqttif_substParam_t *subsParam_stp);

/**---------------------------------------------------------------------------------------
 * @brief     Allocate subscribe handle
 * @author    S. Wink
 * @date      25. Mar. 2019
 * @param     subsParam_stp         user structure for mqtt subs/pubs
 * @return    in case of success an opaque pointer to the handle, else NULL
*//*-----------------------------------------------------------------------------------*/
extern mqttdrv_subsHdl_t mqttdrv_AllocSubs_xp(mqttif_substParam_t *subsParam_stp);

/**---------------------------------------------------------------------------------------
 * @brief     Deallocate subscribe handle
 * @author    S. Wink
 * @date      25. Mar. 2019
 * @param     subsHdl_xp         opaque pointer to the subsription handle
 * @return    in case of success an opaque pointer to the handle, else NULL
*//*-----------------------------------------------------------------------------------*/
extern esp_err_t mqttdrv_DeAllocSub_st(mqttdrv_subsHdl_t subsHdl_xp);

/**---------------------------------------------------------------------------------------
 * @brief     Subscribe to a MQTT message
 * @author    S. Wink
 * @date      25. Mar. 2019
 * @param     subsHdl_xp         opaque pointer to the subsription handle
 * @return    ESP_OK if success, else ESP_FAIL
*//*-----------------------------------------------------------------------------------*/
extern esp_err_t mqttdrv_Subscribe_xp(mqttdrv_subsHdl_t subsHdl_xp);

/**---------------------------------------------------------------------------------------
 * @brief     Un-Subscribe MQTT message
 * @author    S. Wink
 * @date      25. Mar. 2019
 * @param     subsHdl_xp         subscription handle
 * @return    ESP_OK if the message was successful un-subscribed
*//*-----------------------------------------------------------------------------------*/
extern esp_err_t mqttdrv_UnSubscribe(mqttdrv_subsHdl_t subsHdl_xp);

/**---------------------------------------------------------------------------------------
 * @brief     Get the number of all allocated subscriptions in the list
 * @author    S. Wink
 * @date      20. May. 2019
 * @return    number of subscriptions
*//*-----------------------------------------------------------------------------------*/
extern uint8_t mqttdrv_GetNumberOfSubscriptions(void);

/**---------------------------------------------------------------------------------------
 * @brief     Un-Subscribe MQTT message
 * @author    S. Wink
 * @date      25. Mar. 2019
 * @param     subsHdl_xp         subscription handle
 * param      msg_st             message to be published
 * @return    ESP_OK if the message was successful un-subscribed
*//*-----------------------------------------------------------------------------------*/
extern esp_err_t mqttdrv_Publish_st(mqttif_msg_t *msg_stp, uint32_t timeOut_u32);

/****************************************************************************************/
/* Global data definitions: */

#endif

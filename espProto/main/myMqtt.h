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
#ifndef MYMQTT_H
#define MYMQTT_H
/****************************************************************************************/
/* Imported header files: */

#include "mqttif.h"
#include "stdint.h"
#include "esp_err.h"

#include "stdbool.h"


/****************************************************************************************/
/* Global constant defines: */
#define myMqtt_MAX_HOST_NAME_LEN    64U
#define myMqtt_MAX_USER_NAME_LEN    20U
#define myMqtt_MAX_USER_PWD_LEN     20U
#define myMqtt_MAX_SIZE_OF_TOPIC    64U

/****************************************************************************************/
/* Global function like macro defines (to be avoided): */

/****************************************************************************************/
/* Global type definitions (enum (en), struct (st), union (un), typedef (tx): */
typedef struct myMqtt_param_tag
{
    uint8_t host_u8a[myMqtt_MAX_HOST_NAME_LEN];     /*!< MQTT server domain (ipv4 as string) */
    uint32_t port_u32;                              /*!< MQTT server port */
    uint8_t userName_u8a[myMqtt_MAX_USER_NAME_LEN]; /*!< MQTT username */
    uint8_t userPwd_u8a[myMqtt_MAX_USER_PWD_LEN];   /*!< MQTT password */
}myMqtt_param_t;

typedef struct myMqtt_substParam_tag
{
    uint8_t topic_u8a[myMqtt_MAX_SIZE_OF_TOPIC];
    uint32_t qos_u32;
    mqttif_Connected_td conn_fp;
    mqtt_Disconnected_td discon_fp;
    mqtt_DataReceived_td dataRecv_fp;
}myMqtt_substParam_t;

typedef struct myMqtt_obj_tag* mqtt_subsHdl_t;
/****************************************************************************************/
/* Global function definitions: */

/**---------------------------------------------------------------------------------------
 * @brief     Initializes the initialization structure of the mqtt module
 * @author    S. Wink
 * @date      25. Mar. 2019
 * @param     param_stp         pointer to the configuration structure
 * @return    n/a
*//*-----------------------------------------------------------------------------------*/
extern esp_err_t myMqtt_InitializeParameter(myMqtt_param_t *param_stp);

/**---------------------------------------------------------------------------------------
 * @brief     Initialization of the mqtt module
 * @author    S. Wink
 * @date      25. Mar. 2019
 * @param     param_stp         pointer to the configuration structure
 * @return    n/a
*//*-----------------------------------------------------------------------------------*/
extern esp_err_t myMqtt_Initialize(myMqtt_param_t *param_stp);

/**---------------------------------------------------------------------------------------
 * @brief     Connects to the MQTT broker
 * @author    S. Wink
 * @date      25. Mar. 2019
 * @return    n/a
*//*-----------------------------------------------------------------------------------*/
extern esp_err_t myMqtt_Connect(void);

/**---------------------------------------------------------------------------------------
 * @brief     Disconnects from the MQTT broker
 * @author    S. Wink
 * @date      25. Mar. 2019
 * @return    n/a
*//*-----------------------------------------------------------------------------------*/
extern esp_err_t myMqtt_Disconnect(void);

/**---------------------------------------------------------------------------------------
 * @brief     Initialize subscription parameter
 * @author    S. Wink
 * @date      25. Mar. 2019
 * @param     subsParam_stp         user structure for mqtt subs/pubs
 * @return    n/a
*//*-----------------------------------------------------------------------------------*/
extern esp_err_t myMqtt_InitSubParam(myMqtt_substParam_t *subsParam_stp);

/**---------------------------------------------------------------------------------------
 * @brief     Allocate subscribe handle
 * @author    S. Wink
 * @date      25. Mar. 2019
 * @param     subsParam_stp         user structure for mqtt subs/pubs
 * @return    in case of success an opaque pointer to the handle, else NULL
*//*-----------------------------------------------------------------------------------*/
extern mqtt_subsHdl_t myMqtt_AllocSub_xp(myMqtt_substParam_t *subsParam_stp);

/**---------------------------------------------------------------------------------------
 * @brief     Deallocate subscribe handle
 * @author    S. Wink
 * @date      25. Mar. 2019
 * @param     subsHdl_xp         opaque pointer to the subsription handle
 * @return    in case of success an opaque pointer to the handle, else NULL
*//*-----------------------------------------------------------------------------------*/
extern esp_err_t myMqtt_DeAllocSub_st(mqtt_subsHdl_t subsHdl_xp);

/**---------------------------------------------------------------------------------------
 * @brief     Subscribe to a MQTT message
 * @author    S. Wink
 * @date      25. Mar. 2019
 * @param     subsHdl_xp         opaque pointer to the subsription handle
 * @return    ESP_OK if success, else ESP_FAIL
*//*-----------------------------------------------------------------------------------*/
extern esp_err_t myMqtt_Subscribe_xp(mqtt_subsHdl_t subsHdl_xp);

/**---------------------------------------------------------------------------------------
 * @brief     Un-Subscribe MQTT message
 * @author    S. Wink
 * @date      25. Mar. 2019
 * @param     subsHdl_xp         subscription handle
 * @return    ESP_OK if the message was successful un-subscribed
*//*-----------------------------------------------------------------------------------*/
extern esp_err_t myMqtt_UnSubscribe(mqtt_subsHdl_t subsHdl_xp);

/****************************************************************************************/
/* Global data definitions: */

#endif

/*****************************************************************************************
* FILENAME :        gendev.h
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
#ifndef GENDEV_H_
#define GENDEV_H_

/****************************************************************************************/
/* Imported header files: */
#include "mqttif.h"
#include "stdint.h"
#include "esp_err.h"

#include "stdbool.h"
/****************************************************************************************/
/* Global constant defines: */

/****************************************************************************************/
/* Global function like macro defines (to be avoided): */

/****************************************************************************************/
/* Global type definitions (enum (en), struct (st), union (un), typedef (tx): */

typedef struct gendev_param_tag
{
        mqttif_Publish_td publishHandler_fp;
        char *deviceName_chp;
        char *id_chp;
}gendev_param_t;

/****************************************************************************************/
/* Global function definitions: */

/**---------------------------------------------------------------------------------------
 * @brief     Initializes the initialization structure of the generic device module
 * @author    S. Wink
 * @date      25. Mar. 2019
 * @param     param_stp         pointer to the configuration structure
 * @return    n/a
*//*-----------------------------------------------------------------------------------*/
extern esp_err_t gendev_InitializeParameter_st(gendev_param_t *param_stp);

/**---------------------------------------------------------------------------------------
 * @brief     Initialization of the generic device module
 * @author    S. Wink
 * @date      25. Mar. 2019
 * @param     param_stp         pointer to the configuration structure
 * @return    n/a
*//*-----------------------------------------------------------------------------------*/
extern esp_err_t gendev_Initialize_st(gendev_param_t *param_stp);

/**---------------------------------------------------------------------------------------
 * @brief     Get subscribe topics from the generic device
 * @author    S. Wink
 * @date      25. Mar. 2019
 * @param     idx_u16   index of topic, can be used as iterator
 * @return    returns pointer to topic or NULL in case of out of bounce
*//*-----------------------------------------------------------------------------------*/
extern const char* gendev_GetSubsTopics_cchp(uint16_t idx_u16);

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
                                                mqttif_substParam_t *dest_stp);

/**---------------------------------------------------------------------------------------
 * @brief     Get the mqtt connection success function handler
 * @author    S. Wink
 * @date      25. Mar. 2019
 * @return    returns a function handler, NULL if not supported
*//*-----------------------------------------------------------------------------------*/
extern mqttif_Connected_td gendev_GetConnectHandler_fp(void);

/**---------------------------------------------------------------------------------------
 * @brief     Get the mqtt disconnection success function handler
 * @author    S. Wink
 * @date      25. Mar. 2019
 * @return    returns a function handler, NULL if not supported
*//*-----------------------------------------------------------------------------------*/
extern mqttif_Disconnected_td gendev_GetDisconnectHandler_fp(void);

/**---------------------------------------------------------------------------------------
 * @brief     Get the mqtt data received function handler
 * @author    S. Wink
 * @date      25. Mar. 2019
 * @return    returns a function handler, NULL if not supported
*//*-----------------------------------------------------------------------------------*/
extern mqttif_DataReceived_td gendev_GetDataReceivedHandler_fp(void);

/**---------------------------------------------------------------------------------------
 * @brief     Activate the generic device function
 * @author    S. Wink
 * @date      25. Mar. 2019
 * @return    returns ESP_OK if success, else ESP_FAIL
*//*-----------------------------------------------------------------------------------*/
extern esp_err_t gendev_Activate_st(void);

/**---------------------------------------------------------------------------------------
 * @brief     Stopps the generic device function
 * @author    S. Wink
 * @date      25. Mar. 2019
 * @return    returns ESP_OK if success, else ESP_FAIL
*//*-----------------------------------------------------------------------------------*/
extern esp_err_t gendev_Stopp_st(void);

/****************************************************************************************/
/* Global data definitions: */


#endif /* GENDEV_H_ */

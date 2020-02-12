/*****************************************************************************************
* FILENAME :        gendev.h
*
* SHORT DESCRIPTION:
*   This is the header file for the generic mqtt device module
*
* DETAILED DESCRIPTION :
* General usage of the module is :
*       1. generate a parameter instance
*       2. run gendev_InitializeParameter_st() to default set the parameters
*       3. set application specific parameters in the parameter instance
*       4. run gendev_Initialize_st()
*       5. get the subscription objects from gendev_GetSubscriptionByIndex_bol() by 
*               looping thru the function with an increased index until the function
*               returns a null pointer
*       6. activate the module by running gendev_Activate_st()
*       
*
* AUTHOR :    Stephan Wink        CREATED ON :    24. Jan. 2019
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
    uint8_t id_u8;
}gendev_param_t;

/****************************************************************************************/
/* Global function definitions: */

/**--------------------------------------------------------------------------------------
 * @brief     Initializes the initialization structure of the generic device module
 * @param     param_stp         pointer to the configuration structure
 * @return    n/a
*//*-----------------------------------------------------------------------------------*/
extern esp_err_t gendev_InitializeParameter_st(gendev_param_t *param_stp);

/**--------------------------------------------------------------------------------------
 * @brief     Initialization of the generic device module
 * @param     param_stp         pointer to the configuration structure
 * @return    n/a
*//*-----------------------------------------------------------------------------------*/
extern esp_err_t gendev_Initialize_st(gendev_param_t *param_stp);

/**--------------------------------------------------------------------------------------
 * @brief     Get subscriptions from the generic device by index
 * @param     idx_u16    index of topic, can be used as iterator
 * @param     dest_stp   pointer to the subscription parameter structure
 * @return    returns false if the index was out of bounce and no subscription was
 *              copied
*//*-----------------------------------------------------------------------------------*/
extern bool gendev_GetSubscriptionByIndex_bol(uint16_t idx_u16,
                                                mqttif_substParam_t *dest_stp);

/**--------------------------------------------------------------------------------------
 * @brief     Activate the generic device function
 * @return    returns ESP_OK if success, else ESP_FAIL
*//*-----------------------------------------------------------------------------------*/
extern esp_err_t gendev_Activate_st(void);

/**--------------------------------------------------------------------------------------
 * @brief     Deactivates the generic device function
 * @author    S. Wink
 * @date      25. Mar. 2019
 * @return    returns ESP_OK if success, else ESP_FAIL
*//*-----------------------------------------------------------------------------------*/
extern esp_err_t gendev_Deactivate_st(void);

/****************************************************************************************/
/* Global data definitions: */

#endif /* GENDEV_H_ */

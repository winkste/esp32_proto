/*****************************************************************************************
* FILENAME :        controlTask.h
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
#ifndef UDPLOG_H_
#define UDPLOG_H_

/****************************************************************************************/
/* Imported header files: */

#include "esp_system.h"
#include "esp_log.h"
#include "esp_err.h"

#include <string.h>

/****************************************************************************************/
/* Global constant defines: */

/****************************************************************************************/
/* Global function like macro defines (to be avoided): */

/****************************************************************************************/
/* Global type definitions (enum (en), struct (st), union (un), typedef (tx): */

typedef struct udpLog_param_tag
{
    const char *ipAddr_cchp;
    uint32_t conPort_u32;
}udpLog_param_t;

/****************************************************************************************/
/* Global function definitions: */

/**---------------------------------------------------------------------------------------
 * @brief     Initializes the initialization structure of the udplog module
 * @author    S. Wink
 * @date      24. Apr. 2020
 * @param     param_stp     parameter initialization structure
 * @return    ESP_OK in case of success, else error code
*//*-----------------------------------------------------------------------------------*/
extern esp_err_t udpLog_InitializeParameter_st(udpLog_param_t *param_stp);

/**---------------------------------------------------------------------------------------
 * @brief     Initialization of UDP logging
 * @author    S. Wink
 * @date      24. Mar. 2019
 * @param     param_stp     parameter initialization structure
 * @return    ESP_OK in case of success, else error code
*//*-----------------------------------------------------------------------------------*/
extern esp_err_t udpLog_Initialize_st(udpLog_param_t *param_stp);

/**---------------------------------------------------------------------------------------
 * @brief     Function to switch back to original logging
 * @author    S. Wink
 * @date      24. Mar. 2019
 * @param     list_st       variable argument list
 * @return    ESP_OK in case of success, else error code
*//*-----------------------------------------------------------------------------------*/
extern esp_err_t udpLog_Free_st(void);

/****************************************************************************************/
/* Global data definitions: */
#endif



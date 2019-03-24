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

/****************************************************************************************/
/* Global function definitions: */

/**---------------------------------------------------------------------------------------
 * @brief     Function to switch back to original logging
 * @author    S. Wink
 * @date      24. Mar. 2019
 * @param     list_st       variable argument list
 * @return    n/a
*//*-----------------------------------------------------------------------------------*/
extern void udpLog_Free_vd(void);

/**---------------------------------------------------------------------------------------
 * @brief     Initialization of UDP logging
 * @author    S. Wink
 * @date      24. Mar. 2019
 * @param     ipAddr_cchp       IP address of logging server
 * @param     port              port number of logging server
 * @return    ESP_OK in case of success, else error code
*//*-----------------------------------------------------------------------------------*/
extern esp_err_t udpLog_Init_st(const char *ipAddr_cchp, unsigned long port);

/****************************************************************************************/
/* Global data definitions: */
#endif



/*****************************************************************************************
* FILENAME :        logcfg.h
*
* SHORT DESCRIPTION:
*   Header file for logging configuration module.
*
* DETAILED DESCRIPTION :
*       
*
* AUTHOR :    Stephan Wink        CREATED ON :    31. Mar. 2020
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
#ifndef LOGCFG_H
#define LOGCFG_H

#ifdef __cplusplus
extern "C"
{
#endif
/****************************************************************************************/
/* Imported header files: */

#include "stdint.h"
#include "stdbool.h"
#include "stddef.h"
#include "string.h"
#include "esp_log.h"
#include "esp_err.h"

/****************************************************************************************/
/* Global constant defines: */

/****************************************************************************************/
/* Global function like macro defines (to be avoided): */

/****************************************************************************************/
/* Global type definitions (enum (en), struct (st), union (un), typedef (tx): */
typedef enum logcfg_logModuls_tag
{
    logcfg_WIFI,
    logcfg_DEVICES,
    logcfg_MQTT,
    logcfg_ALL,
    logcfg_NONE
}logcfg_logModuls_t;

/****************************************************************************************/
/* Global function definitions: */

/**--------------------------------------------------------------------------------------
 * @brief     configuration of the logging 
 * @param[in]   cfg_en  log configuration
 * @return    ESP_OK in case of success, else ESP_FAIL
*//*-----------------------------------------------------------------------------------*/
extern esp_err_t logcfg_Configure_st(logcfg_logModuls_t cfg_en);

/****************************************************************************************/
/* Global data definitions: */

#ifdef __cplusplus
}
#endif

#endif //BASIC_H

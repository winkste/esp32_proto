/*****************************************************************************************
* FILENAME :        wifiAp.h
*
* DESCRIPTION :
*      Header file for wifi access point module
*
* AUTHOR :    Stephan Wink        CREATED ON :    15.09.2019
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
*****************************************************************************************/
#ifndef WIFIAP_H_
#define WIFIAP_H_

/****************************************************************************************/
/* Imported header files: */

#include "wifiIf.h"
#include "esp_event_legacy.h"

/****************************************************************************************/
/* Global constant defines: */

/****************************************************************************************/
/* Global function like macro defines (to be avoided): */

/****************************************************************************************/
/* Global type definitions (enum (en), struct (st), union (un), typedef (tx): */

/****************************************************************************************/
/* Global function definitions: */

/**---------------------------------------------------------------------------------------
 * @brief     Function to start the wifi 
 * @author    S. Wink
 * @date      24. Jan. 2019
 * @return    ESP_OK if start was successful, else ESP_FAIL
*//*-----------------------------------------------------------------------------------*/
extern esp_err_t wifiAp_Start_st(void);

/**---------------------------------------------------------------------------------------
 * @brief     Function to stop the wifi
 * @author    S. Wink
 * @date      24. Jan. 2019
 * @return    ESP_OK if start was successful, else ESP_FAIL
*//*-----------------------------------------------------------------------------------*/
extern esp_err_t wifiAp_Stop_st(void);

/**--------------------------------------------------------------------------------------
 * @brief     Event converter
 * @author    S. Wink
 * @date      15. Sep. 2019
 * @param     event_stp     the received event
 * @return    converted event id fitting to the wifi module
*//*-----------------------------------------------------------------------------------*/
extern uint32_t wifiAp_EventConverter_u32(system_event_t *event_stp);
/****************************************************************************************/
/* Global data definitions: */

#endif

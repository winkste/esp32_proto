/*****************************************************************************************
* FILENAME :        wifiCtrl.h
*
* DESCRIPTION :
*      Header file for wifi control module
*
* AUTHOR :    Stephan Wink        CREATED ON :    24.01.2019
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
#ifndef WIFICTRL_H_
#define WIFICTRL_H_

/****************************************************************************************/
/* Imported header files: */

#include "wifiIf.h"
#include "stdint.h"

/****************************************************************************************/
/* Global constant defines: */

/****************************************************************************************/
/* Global function like macro defines (to be avoided): */

/****************************************************************************************/
/* Global type definitions (enum (en), struct (st), union (un), typedef (tx): */

typedef struct wifiCtrl_serviceObj_tag* wifiCtrl_serviceHdl_t;
/****************************************************************************************/
/* Global function definitions: */

/**---------------------------------------------------------------------------------------
 * @brief     General initialization of wifi module. This function has to be
 *              executed before the other ones.
 * @author    S. Wink
 * @date      24. Jan. 2019
 * @param     service_stp           wifi service callback
 * @return    ESP_OK if init was successful, else ESP_FAIL
*//*-----------------------------------------------------------------------------------*/
extern esp_err_t wifiCtrl_Initialize_st(wifiIf_service_t *service_stp);

/**---------------------------------------------------------------------------------------
 * @brief     Function to start the wifi 
 * @author    S. Wink
 * @date      24. Jan. 2019
 * @return    ESP_OK if start was successful, else ESP_FAIL
*//*-----------------------------------------------------------------------------------*/
extern esp_err_t wifiCtrl_Start_st(void);

/**---------------------------------------------------------------------------------------
 * @brief     Function to stop the wifi 
 * @author    S. Wink
 * @date      24. Jan. 2019
 * @return    ESP_OK if start was successful, else ESP_FAIL
*//*-----------------------------------------------------------------------------------*/
extern esp_err_t wifiCtrl_Stop_st(void);

/**---------------------------------------------------------------------------------------
 * @brief     Function to register WIFI commands
 * @author    S. Wink
 * @date      24. Jan. 2019
*//*-----------------------------------------------------------------------------------*/
extern void wifiCtrl_RegisterWifiCommands(void);

/**---------------------------------------------------------------------------------------
 * @brief     Function to get the received IP address
 * @author    S. Wink
 * @date      20. Apr. 2020
* @param     ipAdr_cp     pointer to charactor storage location
* @return    length of ip string, zero if not available
*//*-----------------------------------------------------------------------------------*/
extern uint32_t wifiCtrl_GetIpAdress_u32(char *ipAdr_cp);

/****************************************************************************************/
/* Global data definitions: */

#endif

/*****************************************************************************************
* FILENAME :        wifiDrv.h
*
* DESCRIPTION :
*      Header file for wifi driver module
*
* AUTHOR :    Stephan Wink        CREATED ON :    01.09.2019
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
#ifndef WIFIDRV_H_
#define WIFIDRV_H_

/****************************************************************************************/
/* Imported header files: */

#include "wifiIf.h"

/****************************************************************************************/
/* Global constant defines: */

/****************************************************************************************/
/* Global function like macro defines (to be avoided): */

/****************************************************************************************/
/* Global type definitions (enum (en), struct (st), union (un), typedef (tx): */

/****************************************************************************************/
/* Global function definitions: */

/**---------------------------------------------------------------------------------------
 * @brief     Initializes the initialization structure of the mqtt module
 * @author    S. Wink
 * @date      01. Sep. 2019
 * @param     param_stp           wifi parameter structure
*//*-----------------------------------------------------------------------------------*/
extern esp_err_t wifiDrv_InitializeParameter(wifiIf_eventCallB2_t *param_stp);

/**---------------------------------------------------------------------------------------
 * @brief     General initialization of wifi driver. This function has to be
 *              executed before the other ones.
 * @author    S. Wink
 * @date      01. Sep. 2019
 * @param     param_stp           wifi parameter structure
*//*-----------------------------------------------------------------------------------*/
extern esp_err_t wifiDrv_Initialize_vd(wifiIf_eventCallB2_t *param_stp);

/**---------------------------------------------------------------------------------------
 * @brief     Starts the wifi demon if it was initialized
 * @author    S. Wink
 * @date      01. Sep. 2019
*//*-----------------------------------------------------------------------------------*/
extern void wifidrv_StartWifiDemon(void);

/**---------------------------------------------------------------------------------------
 * @brief     Function to register WIFI commands
 * @author    S. Wink
 * @date      01. Sep. 2019
*//*-----------------------------------------------------------------------------------*/
extern void wifiDrv_RegisterWifiCommands_vd(void);
/****************************************************************************************/
/* Global data definitions: */

#endif

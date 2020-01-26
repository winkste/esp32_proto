/*****************************************************************************************
* FILENAME :        controlTask.h
*
* DESCRIPTION :
*       Class header for BME280 sensor
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

#ifndef MAIN_CONTROLTASK_H_
#define MAIN_CONTROLTASK_H_

/****************************************************************************************/
/* Imported header files: */
#include "esp_err.h"

/****************************************************************************************/
/* Global constant defines: */

/****************************************************************************************/
/* Global function like macro defines (to be avoided): */

/****************************************************************************************/
/* Global type definitions (enum, struct, union): */

/****************************************************************************************/
/* Global function definitions: */

/**---------------------------------------------------------------------------------------
 * @brief     Initialization function for control task
 * @author    S. Wink
 * @date      24. Jan. 2019
 * @return    esp error code (ESP_OK, ESP_...)
*//*-----------------------------------------------------------------------------------*/
extern esp_err_t    controlTask_Initialize_st(void);

/**---------------------------------------------------------------------------------------
 * @brief     task routine for the control handling
 * @author    S. Wink
 * @date      24. Jan. 2019
 * @param     pvParameters      interface variable from freertos
*//*-----------------------------------------------------------------------------------*/
extern void         controlTask_Task_vd(void *pvParameters);

//extern void         controlTask_InitializeWifiStaCb(void *data_vp);
//extern void         controlTask_InitializeWifiApCb(void *data_vp);

/****************************************************************************************/
/* Global data definitions: */

#endif /* MAIN_CONTROLTASK_H_ */

/*****************************************************************************************
* FILENAME :        devmgr.h
*
* DESCRIPTION :
*       Header file for device manager
*
* Date: 24. April 2019
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
#ifndef DEVMGR_H_
#define DEVMGR_H_

/****************************************************************************************/
/* Imported header files: */

#include "stdint.h"
#include "esp_err.h"

#include "stdbool.h"

/****************************************************************************************/
/* Global constant defines: */

/****************************************************************************************/
/* Global function like macro defines (to be avoided): */

/****************************************************************************************/
/* Global type definitions (enum (en), struct (st), union (un), typedef (tx): */

typedef struct devmgr_param_tag
{
}devmgr_param_t;

/****************************************************************************************/
/* Global function definitions: */

/**---------------------------------------------------------------------------------------
 * @brief     Initializes the initialization structure of the device manager module
 * @author    S. Wink
 * @date      24. Apr. 2019
 * @param     param_stp         pointer to the configuration structure
 * @return    n/a
*//*-----------------------------------------------------------------------------------*/
extern esp_err_t devmgr_InitializeParameter(devmgr_param_t *param_stp);

/**---------------------------------------------------------------------------------------
 * @brief     Initialization of the device manager module
 * @author    S. Wink
 * @date      24. Apr. 2019
 * @param     param_stp         pointer to the configuration structure
 * @return    n/a
*//*-----------------------------------------------------------------------------------*/
extern esp_err_t devmgr_Initialize(devmgr_param_t *param_stp);

/**---------------------------------------------------------------------------------------
 * @brief     Starts the devices which are setup per parameter
 * @author    S. Wink
 * @date      24. Apr. 2019
 * @return    n/a
*//*-----------------------------------------------------------------------------------*/
extern void devmgr_GenerateDevices(void);

/****************************************************************************************/
/* Global data definitions: */

#endif


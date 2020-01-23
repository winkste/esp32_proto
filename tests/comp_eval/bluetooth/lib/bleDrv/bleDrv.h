/*****************************************************************************************
* FILENAME :        bleDrv.h
*
* SHORT DESCRIPTION:
*   Header file for basic module.
*
* DETAILED DESCRIPTION :
*       
*
* AUTHOR :    Stephan Wink        CREATED ON :    13. Jan. 2019
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
#ifndef BLEDRV_H
#define BLEDRV_H

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

#include "mijaProcl.h"

/****************************************************************************************/
/* Global constant defines: */

/****************************************************************************************/
/* Global function like macro defines (to be avoided): */

/****************************************************************************************/
/* Global type definitions (enum (en), struct (st), union (un), typedef (tx): */

typedef void (* bleDrv_DataAvailable_td)(mijaProcl_parsedData_t *data_st);

typedef struct bleDrv_param_tag
{
    uint32_t scanDurationInSec_u32;
    uint32_t cycleTimeInSec_u32;
    bleDrv_DataAvailable_td dataCb_fp;
}bleDrv_param_t;

/****************************************************************************************/
/* Global function definitions: */

/**--------------------------------------------------------------------------------------
 * @brief     pre-configure the initialization parameter of the module
 * @author    S. Wink
 * @date      13. Jan. 2020
 * @param     param_stp             allocated pointer to the initialization parameters
 * @return    ESP_OK in case of success, else ESP_FAIL
*//*-----------------------------------------------------------------------------------*/
extern esp_err_t bleDrv_InitializeParameter(bleDrv_param_t *param_stp);

/**--------------------------------------------------------------------------------------
 * @brief     initialization of the bleDrv module
 * @author    S. Wink
 * @date      13. Jan. 2020
 * @param     param_stp             allocated pointer to the initialization parameters
 * @return    ESP_OK in case of success, else ESP_FAIL
*//*-----------------------------------------------------------------------------------*/
extern esp_err_t bleDrv_Initialize_st(bleDrv_param_t *param_stp);

/**--------------------------------------------------------------------------------------
 * @brief     activation of the bleDrv module
 * @author    S. Wink
 * @date      13. Jan. 2020
 * @return    ESP_OK in case of success, else ESP_FAIL
*//*-----------------------------------------------------------------------------------*/
extern esp_err_t bleDrv_StartScan_st(void);

extern esp_err_t bleDrv_Activate_st(void);

/****************************************************************************************/
/* Global data definitions: */

#ifdef __cplusplus
}
#endif

#endif //BLEDRV_H

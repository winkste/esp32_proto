/*****************************************************************************************
* FILENAME :        cmdExec.h
*
* DESCRIPTION :
*       Header file for console command table handling
*
* Date: 31. December 2019
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
#ifndef CMDEXEC_H
#define CMDEXEC_H

#ifdef __cplusplus
extern "C"
{
#endif
/****************************************************************************************/
/* Imported header files: */

#include <stddef.h>
#include "stdint.h"

#include "esp_err.h"
#include "consoleDef.h"

/****************************************************************************************/
/* Global constant defines: */

/****************************************************************************************/
/* Global function like macro defines (to be avoided): */

/****************************************************************************************/
/* Global type definitions (enum (en), struct (st), union (un), typedef (tx): */

typedef struct cmdExec_param_t
{
    consoleDefs_cmdTable_t *table_stp;
}cmdExec_param_t;

/****************************************************************************************/
/* Global function definitions: */

/**---------------------------------------------------------------------------------------
 * @brief     Initializes the initialization structure of the cmmand table module
 * @author    S. Wink
 * @date      04. Jan. 2020
 * @param     param_stp         pointer to the configuration structure
 * @return    n/a
*//*-----------------------------------------------------------------------------------*/
extern esp_err_t cmdExec_InitializeParameter_td(cmdExec_param_t *param_stp);

/**---------------------------------------------------------------------------------------
 * @brief   initializes the command table structure. 
 * @author  S. Wink
 * @date    04. Jan. 2020
* @param     param_stp         pointer to the configuration structure
 * @return
 *          - ESP_OK on success
 *          - ESP_ERR_NO_MEM if out of memory
 *          - ESP_ERR_INVALID_STATE if not initialized
*//*------------------------------------------------------------------------------------*/
extern esp_err_t cmdExec_Initialize_td(cmdExec_param_t *param_stp);

/**---------------------------------------------------------------------------------------
 * @brief   search for a command in the command table and executes it. 
 * @author  S. Wink
 * @date    04. Jan. 2020
 * @param   *inMsg_vp   pointer to the input message
 * @param   *outMsg_vp  point to the output message
 * @return  length of response command in outMessage buffer
*//*------------------------------------------------------------------------------------*/
extern uint16_t cmdExec_Execute_td(void *inMsg_vp, void *outMsg_vp);

/****************************************************************************************/
/* Global data definitions: */

#ifdef __cplusplus
}
#endif

#endif //CMDEXEC_H

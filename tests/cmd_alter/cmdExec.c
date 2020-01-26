/*****************************************************************************************
* FILENAME :        cmdExec.c
*
* DESCRIPTION :
*       This module handles the command execution
*
* AUTHOR :    Stephan Wink        CREATED ON :    04.01.2020
*
* PUBLIC FUNCTIONS :
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
*
*****************************************************************************************/

/****************************************************************************************/
/* Include Interfaces */
#include "cmdExec.h"
#include <stddef.h>
#include "stdint.h"

#include "esp_err.h"
#include "consoleDef.h"
#include "utils.h"

/****************************************************************************************/
/* Local constant defines */

#define MODULE_TAG                  "cmdExec"

/****************************************************************************************/
/* Local function like makros */

#define CHECK_EXE(arg) utils_CheckAndLogExecution_bol(MODULE_TAG, arg, __LINE__)

/****************************************************************************************/
/* Local type definitions (enum, struct, union) */

typedef enum objectState_tag
{
    STATE_NOT_INITIALIZED,
    STATE_INITIALIZED
}objectState_t;

typedef struct objectData_tag
{
    objectState_t state_en;
    consoleDefs_cmdTable_t *table_stp;
}objectData_t;

/****************************************************************************************/
/* Local functions prototypes: */
static uint16_t ParseCommandInMessage_u16(void *inMsg_vp);
uint16_t BuildResponse_u16(void *outMsg_vp);

/****************************************************************************************/
/* Local variables: */
static const char *TAG = MODULE_TAG;

static objectData_t hdl_sts = 
{
    .state_en = STATE_NOT_INITIALIZED,
    .table_stp = NULL,
};

/****************************************************************************************/
/* Global functions (unlimited visibility) */

/**---------------------------------------------------------------------------------------
 * @brief     Initializes the initialization structure of the cmmand table module
*//*-----------------------------------------------------------------------------------*/
esp_err_t cmdExec_InitializeParameter_td(cmdExec_param_t *param_stp)
{
    esp_err_t result_st = ESP_FAIL;

    if(NULL != param_stp)
    {
        result_st = ESP_OK;
        param_stp->table_stp = NULL;
    }

    return(result_st);
}

/**---------------------------------------------------------------------------------------
 * @brief   initializes the command table structure. 
*//*------------------------------------------------------------------------------------*/
esp_err_t cmdExec_Initialize_td(cmdExec_param_t *param_stp)
{
    esp_err_t result_st = ESP_FAIL;

    if(NULL != param_stp)
    {
        if(NULL != param_stp->table_stp)
        {
            result_st = ESP_OK;
            hdl_sts.table_stp = param_stp->table_stp;
            hdl_sts.state_en = STATE_NOT_INITIALIZED;
        }
    }

    return(result_st);
}

/**---------------------------------------------------------------------------------------
 * @brief   search for a command in the command table and executes it. 
*//*------------------------------------------------------------------------------------*/
uint16_t cmdExec_Execute_td(void *inMsg_vp, void *outMsg_vp)
{
    uint16_t retLength_u16 = 0U;
    uint16_t cmdIdx_u16 = 0xFFFF;
    uint16_t ack_u16 = 0U;

    if(     (STATE_INITIALIZED != hdl_sts.state_en) 
        &&  (NULL != inMsg_vp) && (NULL != outMsg_vp))
    {
        cmdIdx_u16 = ParseCommandInMessage_u16(inMsg_vp);

        if(0xFFFF != cmdIdx_u16)
        {
            // valid command found, execute the command
            (hdl_sts.table_stp + cmdIdx_u16)->cmdFunc_f(&retLength_u16, &ack_u16, 
                                                        (uint8_t *)inMsg_vp + consoleDef_REQ_DATA_ADR, 
                                                        (uint8_t*)outMsg_vp + consoleDef_RESP_DATA_ADR);
        }

        if(0U != retLength_u16)
        {
            retLength_u16 = BuildResponse_u16(outMsg_vp);
        }


    }
    else
    {
        // no execution possible, no response possible
        retLength_u16 = 0U;
    }

    return(retLength_u16);
}

/****************************************************************************************/
/* Local functions: */

/**---------------------------------------------------------------------------------------
 * @brief   search for a command in the command table and executes it. 
 * @author  S. Wink
 * @date    04. Jan. 2020
 * @param   *inMsg_vp   pointer to the input message
 * @return  index of found command or consoleDef_CMD_TABLE_END
*//*------------------------------------------------------------------------------------*/
static uint16_t ParseCommandInMessage_u16(void *inMsg_vp)
{
    uint16_t cmdIdx_u16 = 0U;
    uint8_t *inMsg_u8p = (uint8_t *)inMsg_vp;

    while(consoleDef_CMD_TABLE_END != (hdl_sts.table_stp + cmdIdx_u16)->cmdId_u16)
    {
        if(*(uint16_t *)(inMsg_u8p + consoleDef_LENGTH_ADR) == (hdl_sts.table_stp + cmdIdx_u16)->cmdId_u16)
        {
            break;
        }
    }

    return(cmdIdx_u16);
}

/**---------------------------------------------------------------------------------------
 * @brief   search for a command in the command table and executes it. 
 * @author  S. Wink
 * @date    04. Jan. 2020
 * @param   *outMsg_vp  point to the output message
 * @return  length of response command in outMessage buffer
*//*------------------------------------------------------------------------------------*/
uint16_t BuildResponse_u16(void *outMsg_vp)
{
    return(0U);
}


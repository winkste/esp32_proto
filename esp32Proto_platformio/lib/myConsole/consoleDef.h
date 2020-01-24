/*****************************************************************************************
* FILENAME :        consoleDef.h
*
* DESCRIPTION :
*       Header file for console command handling definitions
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
#ifndef CONSOLEDEF_H
#define CONSOLEDEF_H

#ifdef __cplusplus
extern "C"
{
#endif
/****************************************************************************************/
/* Imported header files: */

#include <stddef.h>
#include "stdint.h"

/****************************************************************************************/
/* Global constant defines: */

#define consoleDef_CMD_ADR          0x0000
#define consoleDef_LENGTH_ADR       0x0002
#define consoleDef_REQ_DATA_ADR     0x0004
#define consoleDef_ACK_ADR          0x0004
#define consoleDef_RESP_DATA_ADR    0x0006

#define consoleDef_CMD_LENGTH       0x0002
#define consoleDef_LENGTH_LENGTH    0x0002
#define consoleDef_ACK_LENGTH       0x0002
#define consoleDef_MAX_DATA_LENGTH  0x4096
#define consoleDef_MAX_REQ_LENGTH   0x409A
#define consoleDef_MAX_RESP_LENGTH  0x409C

#define consoleDef_GEN_CMDS         0x8000
#define consoleDef_CMD_TABLE_END    0xFFFF

/****************************************************************************************/
/* Global function like macro defines (to be avoided): */

/****************************************************************************************/
/* Global type definitions (enum (en), struct (st), union (un), typedef (tx): */

typedef uint16_t (*consoleDef_cmdFunc_t)(uint16_t *length_u16p, uint16_t *ack_u16p, 
                                            void *src_vp, void *dest_vp);

/**
 * @brief Parameters for console initialization
 */
typedef struct 
{
    uint16_t cmdId_u16;                 //!< command identifier
    consoleDef_cmdFunc_t cmdFunc_f;     //!< function pointer to command
}consoleDefs_cmdTable_t;


/****************************************************************************************/
/* Global function definitions: */

/****************************************************************************************/
/* Global data definitions: */

#ifdef __cplusplus
}
#endif

#endif //MYCONSOLE_H

/*****************************************************************************************
* FILENAME :        cmdTable.h
*
* DESCRIPTION :
*       Header file for console command table 
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
#ifndef CMDTABLE_H
#define CMDTABLE_H

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

/****************************************************************************************/
/* Global function definitions: */

/****************************************************************************************/
/* Global data definitions: */

static const consoleDefs_cmdTable_t cmdTable_TABLE[] = 
{
    consoleDef_GEN_CMDS + 0x0001,   NULL,     // help command to read all available cmds
    consoleDef_CMD_TABLE_END,       NULL,     // end of command table marker
};

#ifdef __cplusplus
}
#endif

#endif //CMDTABLE_H

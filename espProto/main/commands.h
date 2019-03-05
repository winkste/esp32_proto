/*****************************************************************************************
* FILENAME :        commands.h
*
* DESCRIPTION :
*       Header file for command handling and execution
*
* Date: 26. January 2019
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
#ifndef MAIN_COMMANDS_H_
#define MAIN_COMMANDS_H_

/****************************************************************************************/
/* Imported header files: */
#include "stdint.h"
#include "esp_err.h"

/****************************************************************************************/
/* Global constant defines: */

/****************************************************************************************/
/* Global function like macro defines (to be avoided): */

/****************************************************************************************/
/* Global type definitions (enum (en), struct (st), union (un), typedef (tx): */

typedef struct cmdElement_tag
{
    const char *cmd_ccp;
    uint8_t cmdLength_u8;
    void (*commandCallBack_ptrs)(void *userData_vp);
}cmdElement_t;

/****************************************************************************************/
/* Global function definitions: */

extern void commands_Initialize(void);
extern esp_err_t commands_AddElement(cmdElement_t *cmdElem_pst);
extern void commands_Execute(char *buffer_cp, uint16_t length_u16);

/****************************************************************************************/
/* Global data definitions: */

#endif /* MAIN_COMMANDS_H_ */

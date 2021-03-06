/*****************************************************************************************
* FILENAME :        consoleSocket.h
*
* DESCRIPTION :
*       Header file for console socket server handling
*
* AUTHOR :    Stephan Wink        CREATED ON :    25. January 2019
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
#ifndef CONSOLESOCKET_H_
#define CONSOLESOCKET_H_

/****************************************************************************************/
/* Imported header files: */
#include "esp_err.h"

/****************************************************************************************/
/* Global constant defines: */

/****************************************************************************************/
/* Global function like macro defines (to be avoided): */

/****************************************************************************************/
/* Global type definitions (enum (en), struct (st), union (un), typedef (tx): */

typedef struct consoleSocket_parameter_tag
{
    void (*eventSocketError_ptrs)(void);

}socketServer_parameter_t;

/****************************************************************************************/
/* Global function definitions: */

extern esp_err_t consoleSocket_Initialize_st(socketServer_parameter_t *param_stp);
extern void consoleSocket_Task_vd(void *pvParameters);
extern void consoleSocket_Activate_vd(void);
extern void consoleSocket_Deactivate_vd(void);

/****************************************************************************************/
/* Global data definitions: */

#endif

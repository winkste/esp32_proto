/*****************************************************************************************
* FILENAME :        wifiStation.h
*
* DESCRIPTION :
*      Header file for wifi station mode module.
*
* AUTHOR :    Stephan Wink        CREATED ON :    26.08.2019
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
#ifndef WIFISTATION_H_
#define WIFISTATION_H_

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
 * @brief     Function to initialize WIFI to station mode
 * @author    S. Wink
 * @date      26. Aug. 2019
 * @param     wifiSettings_stp           wifi settings structure pointer
 * @return    returns the exectution result of function
*//*-----------------------------------------------------------------------------------*/
extern esp_err_t wifiStation_Initialize_st(wifiIf_setStation_t *setup_stp);

/****************************************************************************************/
/* Global data definitions: */

#endif

/*****************************************************************************************
* FILENAME :        wifiIf.h
*
* DESCRIPTION :
*      Interface module for wifiCtrl
*
* AUTHOR :    Stephan Wink        CREATED ON :    21.08.2019
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
#ifndef WIFIIF_H_
#define WIFIIF_H_

/****************************************************************************************/
/* Imported header files: */
#include "esp_wifi.h"

/****************************************************************************************/
/* Global constant defines: */

/****************************************************************************************/
/* Global function like macro defines (to be avoided): */

/****************************************************************************************/
/* Global type definitions (enum (en), struct (st), union (un), typedef (tx): */
typedef struct wifiIf_eventCallB_tag
{
    void (*eventCallBackOnAPStarted_ptrs)(void);
    void (*eventCallBackOnStationStarted_ptrs)(void);
    void (*eventCallBackWifiDisconn_ptrs)(void);
}wifiIf_eventCallB_t;

typedef struct wifiIf_eventCallB2_tag
{
    void (*eventCallBOnClientConn_fp)(void);
    void (*eventCallBOnStationConn_fp)(void);
    void (*eventCallBackWifiDisconn_fp)(void);
}wifiIf_eventCallB2_t;

typedef struct wifiIf_callBackStation_tag
{
    //void (*callBackStationConStarted_fp)(void);
    void (*callBackStationConnected_fp)(void);
    void (*callBackStationDisconn_fp)(void);
}wifiIf_callBackStation_t;

typedef struct wifiIf_callBackAp_tag
{
    void (*callBackClientConnected_fp)(void);
    void (*callBackClientDisconn_fp)(void);
}wifiIf_callBackAp_t;

typedef struct wifiIf_setStation_tag
{
     wifi_config_t wifiSettings_st;
     wifiIf_callBackStation_t wifiCallBacks_st;
}wifiIf_setStation_t;

typedef struct wifiIf_setAp_tag
{
     wifi_config_t wifiSettings_st;
     wifiIf_callBackAp_t wifiCallBacks_st;
}wifiIf_setAp_t;

/****************************************************************************************/
/* Global function definitions: */

/****************************************************************************************/
/* Global data definitions: */

#endif

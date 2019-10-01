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
#include "sdkconfig.h"

/****************************************************************************************/
/* Global constant defines: */
#define wifiIf_SIZE_OF_SSID_VECTOR         32U
#define wifiIf_SIZE_OF_PASSWORD            64U

#define wifiIf_DEFAULT_WIFI_SSID_STA    CONFIG_WIFI_SSID
#define wifiIf_DEFAULT_WIFI_PASS_STA    CONFIG_WIFI_PASSWORD

#define wifiIf_EVENT_DONT_CARE              0x00000000
#define wifiIf_EVENT_CLIENT_CONNECTED       0x00000001
#define wifiIf_EVENT_CLIENT_DISCONNECTED    0x00000002


#define wifiIf_EVENT_STATION_STOP           0x00000004
#define wifiIf_EVENT_STATION_CONNECTED      0x00000008
#define wifiIf_EVENT_STATION_DISCONNECTED   0x00000010

#define wifiIf_EVENT_CONN_TIMEOUT           0x00000020

/****************************************************************************************/
/* Global function like macro defines (to be avoided): */

/****************************************************************************************/
/* Global type definitions (enum (en), struct (st), union (un), typedef (tx): */

typedef void (*wifiIf_ServiceHandler_fcp)(void);
typedef uint32_t (*wifiIf_Converter_fcp)(system_event_t *event_stp);
typedef struct wifiIf_service_tag
{
        wifiIf_ServiceHandler_fcp OnStationConncetion_fcp;
        wifiIf_ServiceHandler_fcp OnDisconncetion_fcp;
        wifiIf_ServiceHandler_fcp OnClientConnection_fcp;
}wifiIf_service_t;

typedef struct wifiIf_stationParam_tag
{
    uint8_t ssid[wifiIf_SIZE_OF_SSID_VECTOR];           /**< SSID of ESP32 soft-AP */
    uint8_t password[wifiIf_SIZE_OF_PASSWORD];       /**< Password of ESP32 soft-AP */
}wifiIf_stationParam_t;

typedef struct wifiIf_eventLookup_tag
{
    system_event_id_t eventId_st;
    uint32_t wifiEvent_u32;
}wifiIf_eventLookup_t;

/****************************************************************************************/
/* Global function definitions: */

/****************************************************************************************/
/* Global data definitions: */

#endif

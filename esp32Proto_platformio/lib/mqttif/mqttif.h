/*****************************************************************************************
* FILENAME :        mqttif.h
*
* DESCRIPTION :
*       Header file for mqtt interface description
*
* Date: 31. Mar 2019
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
#ifndef MQTTIF_H
#define MQTTIF_H
/****************************************************************************************/
/* Imported header files: */

#include "stdint.h"
#include "esp_err.h"

/****************************************************************************************/
/* Global constant defines: */
#define mqttif_MAX_SIZE_OF_TOPIC    256U
#define mqttif_MAX_SIZE_OF_DATA     256U

/****************************************************************************************/
/* Global function like macro defines (to be avoided): */

/****************************************************************************************/
/* Global type definitions (enum (en), struct (st), union (un), typedef (tx): */
typedef struct mqttif_msg_tag
{
        uint32_t topicLen_u32;
        char *topic_chp;
        uint32_t dataLen_u32;
        char *data_chp;
        int32_t msgId_s32;
        int32_t qos_s32;
        int32_t retain_s32;
}mqttif_msg_t;

typedef void (* mqttif_Connected_td)(void);
typedef void (* mqttif_Disconnected_td)(void);
typedef esp_err_t (* mqttif_DataReceived_td)(mqttif_msg_t *);
typedef esp_err_t (* mqttif_Publish_td)(mqttif_msg_t *, uint32_t);

typedef struct mqttif_substParam_tag
{
    uint8_t topic_u8a[mqttif_MAX_SIZE_OF_TOPIC];
    uint32_t qos_u32;
    mqttif_Connected_td conn_fp;
    mqttif_Disconnected_td discon_fp;
    mqttif_DataReceived_td dataRecv_fp;
}mqttif_substParam_t;

/****************************************************************************************/
/* Global function definitions: */

/****************************************************************************************/
/* Global data definitions: */
#endif

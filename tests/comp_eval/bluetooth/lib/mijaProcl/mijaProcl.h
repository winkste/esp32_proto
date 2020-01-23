/*****************************************************************************************
* FILENAME :        mijaProcl.h
*
* SHORT DESCRIPTION:
*   Header file for mijaProcl module.
*
* DETAILED DESCRIPTION :
*       
*
* AUTHOR :    Stephan Wink        CREATED ON :    13. Jan. 2019
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
*****************************************************************************************/
#ifndef MIJAPROCL_H
#define MIJAPROCL_H

#ifdef __cplusplus
extern "C"
{
#endif
/****************************************************************************************/
/* Imported header files: */

#include "stdint.h"
#include "stdbool.h"
#include "stddef.h"
#include "string.h"

/****************************************************************************************/
/* Global constant defines: */

#define mija_SIZE_MAC_ADDR      6U
/****************************************************************************************/
/* Global function like macro defines (to be avoided): */

/****************************************************************************************/
/* Global type definitions (enum (en), struct (st), union (un), typedef (tx): */

typedef enum mijaProcl_dataType_tag
{
    mija_TYPE_TEMPERATURE   = 0x04,
    mija_TYPE_HUMIDITY      = 0x06,
    mija_TYPE_BATTERY       = 0x0A,
    mija_TYPE_TEMPHUM       = 0x0D,
    mija_TYPE_UNKNOWN       = 0XFF
}mijaProcl_dataType_t;

typedef struct mijaProcl_parsedData_tag
{
    uint16_t uuid_u16;
    uint8_t macAddr_u8a[mija_SIZE_MAC_ADDR];
    uint8_t msgCnt_u8;
    mijaProcl_dataType_t dataType_en;
    float battery_f32;
    float temperature_f32;
    float humidity_f32;
    uint8_t parseResult_u8;
}mijaProcl_parsedData_t;

typedef struct mijaProcl_param_tag
{

}mijaProcl_param_t;

/****************************************************************************************/
/* Global function definitions: */

/**--------------------------------------------------------------------------------------
 * @brief     parses a message string and sets the ouput data structure
 * @author    S. Wink
 * @date      13. Jan. 2020
 * @param     msg_u8p       pointer to input data with the message 
 * @param     outData_stp   pointer to the ouput message data structure 
 * @return    true in case of success, else false
*//*-----------------------------------------------------------------------------------*/
extern bool mijaProcl_ParseMessage_bol(uint8_t *msg_u8p, 
                                            mijaProcl_parsedData_t *outData_stp);

/****************************************************************************************/
/* Global data definitions: */

#ifdef __cplusplus
}
#endif

#endif //MIJAPROCL_H

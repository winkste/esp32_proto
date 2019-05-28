/*****************************************************************************************
* FILENAME :        utils.h
*
* DESCRIPTION :
*       Header file for
*
* Date: 24. January 2019
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

#ifndef UTILS_H
#define UTILS_H

/****************************************************************************************/
/* Imported header files: */

#include "stdint.h"

/****************************************************************************************/
/* Global constant defines: */

/****************************************************************************************/
/* Global function like macro defines (to be avoided): */
#define utils_MIN(x, y) (((x) < (y)) ? (x) : (y))
/****************************************************************************************/
/* Global type definitions (enum (en), struct (st), union (un), typedef (tx): */

/****************************************************************************************/
/* Global function definitions: */

/**---------------------------------------------------------------------------------------
 * @brief     Copies a float value to a character buffer
 * @author    winkste
 * @date      20 Okt. 2017
 * @param     data value as float representation
 * @param     buffer_p    pointer to result buffer string
 * @return    pointer to char buffer
*//*-----------------------------------------------------------------------------------*/
extern char* utils_FloatToString_chp(float value_f32, char* buffer_chp);

/**---------------------------------------------------------------------------------------
 * @brief     Copies a integer value to decimal representive in a character buffer
 * @author    winkste
 * @date      20 Okt. 2017
 * @param     data value as integer representation
 * @param     buffer_p    pointer to result buffer string
 * @return    pointer to char buffer
*//*-----------------------------------------------------------------------------------*/
extern char* utils_IntegerToDecString_chp(int32_t value_s32, char* buffer_chp);

/**---------------------------------------------------------------------------------------
 * @brief     Copies a integer value to decimal representive in a character buffer
 * @author    winkste
 * @date      20 Okt. 2017
 * @param     red_u8      decimal red value
 * @param     green_u8    decimal green value
 * @param     blue_u8     decimal blue value
 * @param     buffer_p    pointer to result buffer string
 * @return    pointer to char buffer
*//*-----------------------------------------------------------------------------------*/
extern char* utils_RGBToString_chp(uint8_t red_u8, uint8_t green_u8, uint8_t blue_u8,
                          char* pBuffer_p);


/**---------------------------------------------------------------------------------------
 * @brief     This function helps to build the complete topic including the
 *              custom device.
 * @author    winkste
 * @date      20 Okt. 2017
 * @param     dev_p       pointer to device string
 * @param     channel_p   pointer to channel string
 * @param     topic_p     pointer to topic string
 * @param     buffer_p    pointer to result buffer string
 * @return    combined topic as char pointer, it uses buffer_p to store the topic
*//*-----------------------------------------------------------------------------------*/
extern char* utils_BuildSendTopicWChan_chp(const char *dev_p, const char *channel_p,
                                      const char *topic_p, char *buffer_p);

/**---------------------------------------------------------------------------------------
 * @brief     This function helps to build the complete topic including the
 *              custom device.
 * @author    winkste
 * @date      20 Okt. 2017
 * @param     dev_cchp       pointer to device string
 * @param     channel_cchp   pointer to channel string
 * @param     topic_cchp     pointer to topic string
 * @param     buffer_chp    pointer to result buffer string
 * @return    combined topic as char pointer, it uses buffer_p to store the topic
*//*-----------------------------------------------------------------------------------*/
extern char* utils_BuildSendTopic_chp(const char *dev_cchp, const char *channel_cchp,
        const char *topic_chp, char *buffer_chp);

/**---------------------------------------------------------------------------------------
 * @brief     This function helps to build the complete topic including the
 *              custom device.
 * @author    winkste
 * @date      20 Okt. 2017
 * @param     dev_p       pointer to device string
 * @param     channel_p   pointer to channel string
 * @param     topic_p     pointer to topic string
 * @param     buffer_p    pointer to result buffer string
 * @return    combined topic as char pointer, it uses buffer_p to store the topic
*//*-----------------------------------------------------------------------------------*/
extern char* utils_BuildReceiveTopic_chp(const char *dev_p, const char *channel_p,
                                      const char *topic_p, char *buffer_p);

/**---------------------------------------------------------------------------------------
 * @brief     This function helps to build the complete topic for broadcast
 *              subscriptions.
 * @author    winkste
 * @date      21 May. 2019
 * @param     topic_p     pointer to topic string
 * @param     buffer_p    pointer to result buffer string
 * @return    combined topic as char pointer, it uses buffer_p to store the topic
*//*-----------------------------------------------------------------------------------*/
extern char* utils_BuildReceiveTopicBCast_chp(const char *topic_p, char *buffer_p);

/**---------------------------------------------------------------------------------------
 * @brief     This function calculates the logarithm digits value (0-1023) based on the
 *              linear  input percentage (0-100%).
 * @author    winkste
 * @date      24 Oct. 2018
 * @param     percent_u8       percentage value 0-100
 * @return    N/A
*//*-----------------------------------------------------------------------------------*/
extern uint16_t utils_CalcLogDigitsFromPercent_u16(uint8_t percent_u8);

/**---------------------------------------------------------------------------------------
 * @brief     This function calculates the logarithm digits value (0-1023) based on the
 *              linear  input percentage (0-100%).
 * @author    winkste
 * @date      24 Oct. 2018
 * @param     percent_u8       percentage value 0-100
 * @param     maxVal_u16       maximum digit value
 * @return    N/A
*//*-----------------------------------------------------------------------------------*/
extern uint16_t utils_CalcLogDigitsFromPercentWMax_u16(uint8_t percent_u8,
                                                    uint16_t maxVal_u16);

/****************************************************************************************/
/* Global data definitions: */
#endif

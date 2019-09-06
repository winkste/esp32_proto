/*****************************************************************************************
* FILENAME :        utils.c
*
* DESCRIPTION :
*       Class implementation for generic Utils
*
* NOTES :
*
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
*
* AUTHOR :    Stephan Wink        START DATE :    09.09.2018
*
*****************************************************************************************/

/****************************************************************************************/
/* Include Interfaces */
#include "utils.h"

#include "stdint.h"
#include "stdbool.h"
#include "stdio.h"
#include "stdlib.h"
#include "math.h"
#include "esp_err.h"
#include "esp_log.h"

/****************************************************************************************/
/* Local constant defines */

/****************************************************************************************/
/* Local function like makros */

/****************************************************************************************/
/* Local type definitions (enum, struct, union) */

/****************************************************************************************/
/* Public functions (unlimited visibility) */

/**---------------------------------------------------------------------------------------
 * @brief     Copies a float value to a character buffer
*//*-----------------------------------------------------------------------------------*/
char* utils_FloatToString_chp(float value_f32, char* buffer_chp)
{
    sprintf(buffer_chp, "%.4f", value_f32);
    return(buffer_chp);
}

/**---------------------------------------------------------------------------------------
 * @brief     Copies a integer value to decimal representive in a character buffer
*//*-----------------------------------------------------------------------------------*/
char* utils_IntegerToDecString_chp(int32_t value_s32, char* buffer_chp)
{
    return itoa(value_s32, buffer_chp, 10);       // call the library function
}

/**---------------------------------------------------------------------------------------
 * @brief     Copies a RGB color code to a string
*//*-----------------------------------------------------------------------------------*/
char* utils_RGBToString_chp(uint8_t red_u8, uint8_t green_u8, uint8_t blue_u8,
                          char* pBuffer_p)
{
    snprintf(pBuffer_p, 20, "%d,%d,%d", red_u8, green_u8, blue_u8);
    return(pBuffer_p);
}


/**---------------------------------------------------------------------------------------
 * @brief     This function helps to build the complete topic including the
 *              custom device.
*//*-----------------------------------------------------------------------------------*/
char* utils_BuildSendTopicWChan_chp(const char *dev_p, const char *channel_p,
                                      const char *topic_p, char *buffer_p)
{
  sprintf(buffer_p, "std/%s/s/%s/%s", dev_p, channel_p, topic_p);
  return buffer_p;
}

/**---------------------------------------------------------------------------------------
 * @brief     This function helps to build the complete topic including the
 *              custom device.
*//*-----------------------------------------------------------------------------------*/
const char* utils_BuildSendTopic_chp(const char *dev_cchp, const char *channel_cchp,
                                      const char *topic_chp, char *buffer_chp)
{
    if(NULL != channel_cchp)
    {
        sprintf(buffer_chp, "std/%s/s/%s/%s", dev_cchp, channel_cchp, topic_chp);
    }
    else
    {
        sprintf(buffer_chp, "std/%s/s/%s", dev_cchp, topic_chp);
    }
  return dev_cchp;
}

/**---------------------------------------------------------------------------------------
 * @brief     This function helps to build the complete topic including the
 *              custom device.
*//*-----------------------------------------------------------------------------------*/
char* utils_BuildReceiveTopic_chp(const char *dev_p, const char *channel_p,
                                      const char *topic_p, char *buffer_p)
{
  sprintf(buffer_p, "std/%s/r/%s/%s", dev_p, channel_p, topic_p);
  return buffer_p;
}

/**---------------------------------------------------------------------------------------
 * @brief     This function helps to build the complete topic for broadcast
 *              subscriptions.
*//*-----------------------------------------------------------------------------------*/
char* utils_BuildReceiveTopicBCast_chp(const char *topic_p, char *buffer_p)
{
  sprintf(buffer_p, "std/bcast/r/%s", topic_p);
  return buffer_p;
}

/**---------------------------------------------------------------------------------------
 * @brief     This function calculates the logarithm digits value (0-1023) based on the
 *              linear  input percentage (0-100%).
*//*-----------------------------------------------------------------------------------*/
uint16_t utils_CalcLogDigitsFromPercent_u16(uint8_t percent_u8)
{
    return(0);
  /*return(uint16_t) ((1023.0F * log10(max((uint8_t)1U, percent_u8)))
                      / log10(100.0) + 0.5F);*/
}

/**---------------------------------------------------------------------------------------
 * @brief     This function calculates the logarithm digits value (0-1023) based on the
 *              linear  input percentage (0-100%).
*//*-----------------------------------------------------------------------------------*/
uint16_t utils_CalcLogDigitsFromPercentWMax_u16(uint8_t percent_u8,
                                                    uint16_t maxVal_u16)
{
    return(0);
  /*return(uint16_t) ((maxVal_u16 * log10(max((uint8_t)1U, percent_u8)))
                      / log10(100.0) + 0.5F);*/
}

/**--------------------------------------------------------------------------------------
 * @brief     Change the state of the internal object based on the wifi state
 * @author    S. Wink
 * @date      28. Aug. 2019
 * @param     file_ccp    name of the file where the check is executed
 * @param     exeCode_st  the returned standard execution code
 * @param     line_u32    the line of code where the execution was done
 * @return    true if error code is ESP_OK, else false
*//*-----------------------------------------------------------------------------------*/
bool utils_CheckAndLogExecution_bol(const char *file_ccp, esp_err_t exeCode_st,
                                        uint32_t line_u32)
{
    if(ESP_OK != exeCode_st)
    {
        ESP_LOGE(file_ccp, "bad function error code returned in line: %d, error code: %d",
                    exeCode_st, line_u32);
    }

    return(ESP_OK == exeCode_st);
}

/**--------------------------------------------------------------------------------------
 * @brief     Change the state of the internal object based on the wifi state
*//*-----------------------------------------------------------------------------------*/
void utils_CheckAndLogExec_vd(const char *file_ccp, esp_err_t exeCode_st,
                                        uint32_t line_u32)
{
    if(ESP_OK != exeCode_st)
    {
        ESP_LOGE(file_ccp, "bad function error code returned in line: %d, error code: %d",
                    exeCode_st, line_u32);
    }
}

/****************************************************************************************/
/* Private functions: */


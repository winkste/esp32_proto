/****************************************************************************************
* FILENAME :        mijaProcl.c
*
* SHORT DESCRIPTION:
*   Source file for mijaProcl module.  
*
* DETAILED DESCRIPTION :   
* This module parses the data message of a mija bluetooth temperature and humidity 
* sensor.
* A message example looks like:
* address:   0  1  2  3  4       5  6    7  8  9 10     11  12 13 14 15 16 17   18  19  20  21 22   23 24
* data:     02 01 06 15 16		95 fe	50 20 aa 01		8e	86 10 37 34 2d 58	0d	10	04	ce 00	b9 01	
*
* with the following interpretation:
*   - UUID address 05 and 06, data always = 0x95fe  
*   - message counter address: 11
*   - device mac address address: 12 - 17,
            data reverse, means MAC address of example: 58:2D:34:37:10:86
    - data type address: 18, types: 
                                TEMPERATURE	            0x04
                                HUMIDITY	            0x06
                                BATTERY	                0x0A
                                TEMPERATURE & HUMIDITY  0x0D
            (example shows a temperature and humidity message)
    - length address of the data: 20 with following length in bytes known:
                                TEMPERATURE	            2
                                HUMIDITY	            2
                                BATTERY	                1
                                TEMPERATURE & HUMIDITY  4
            (example has got a data length of 4 bytes)
    - data: 2 byte data sets are low byte first, here the data is parsed as follows:
            temperature raw   = 0xce00, humidity raw  = 0xb901
            temperature conv  = 0x00ce, humidity conf = 0x01b9
            data to float     = data conv / 10
            temperature float = 20,6 °C, humidity     = 44,1% 
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
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
****************************************************************************************/

/***************************************************************************************/
/* Include Interfaces */

#include "mijaProcl.h"

#include "stdint.h"
#include "stdbool.h"
#include "stddef.h"
#include "string.h"
#include "stdio.h"
//#include "esp_log.h"
//#include "esp_err.h"

/***************************************************************************************/
/* Local constant defines */

#define UUID_DATA_LOW_ADR		            5U
#define UUID_DATA_HIGH_ADR		            6U
#define MSG_CNT_ADR				            11U
#define DEVICE_MAC_ADR                      17U
#define DATA_TYPE_ID_ADR				    18U
#define DATA_LEN_ADR					    20U

#define DATA_BATTERY_LOWBYTE_ADR		    21U

#define DATA_HUMTEMP_TEMP_LOWBYTE_ADR	    21U
#define DATA_HUMTEMP_TEMP_HIGHBYTE_ADR	    22U
#define DATA_HUMTEMP_HUM_LOWBYTE_ADR	    23U
#define DATA_HUMTEMP_HUM_HIGHBYTE_ADR	    24U

#define DATA_TEMP_LOWBYTE_ADR			    21U
#define DATA_TEMP_HIGHBYTE_ADR			    22U

#define DATA_HUM_LOWBYTE_ADR			    21U
#define DATA_HUM_HIGHBYTE_ADR			    22U


#define UUID_DATA_LOW_VAL		            0x95U
#define UUID_DATA_HIGH_VAL		            0xFEU

#define DATA_TYPE_ID_TEMPHUM			    0x0DU
#define DATA_TYPE_ID_BATT				    0x0AU
#define DATA_TYPE_ID_TEMP				    0x04U
#define DATA_TYPE_ID_HUM				    0x06U

#define DATA_LEN_BATTERY_STD			    1U
#define DATA_LEN_TEMP_STD				    2U
#define DATA_LEN_HUM_STD				    2U
#define DATA_LEN_TEMPHUM_STD			    4U

#define RET_CODE_OK						    0U
#define RET_CODE_ERROR_UUID_MISSMATCH	    0x01U
#define RET_CODE_ERROR_UNKNOWN_TYPE		    0x02U
#define RET_CODE_UNEXPECTED_TEMP_LENGTH     0x04U
#define RET_CODE_UNEXPECTED_HUM_LENGTH      0x08U
#define RET_CODE_UNEXPECTED_BAT_LENGTH      0x10U
#define RET_CODE_UNEXPECTED_TEMPHUM_LENGTH  0x20U


/***************************************************************************************/
/* Local function like makros */

/***************************************************************************************/
/* Local type definitions (enum, struct, union) */

/***************************************************************************************/
/* Local functions prototypes: */
static bool CheckTheMsgUuid_bol(uint8_t *msg_u8p, mijaProcl_parsedData_t *outData_stp);
static void ParseTheMsgCounter_vd(uint8_t *msg_u8p, mijaProcl_parsedData_t *outData_stp);
static void ParseMacAddr_vd(uint8_t *msg_u8p, mijaProcl_parsedData_t *outData_stp);
static bool ParseDataType_bol(uint8_t *msg_u8p, mijaProcl_parsedData_t *outData_stp);

static bool ConvertTemperature_bol(uint8_t *msg_u8p, 
                                    mijaProcl_parsedData_t *outData_stp);
static bool ConvertHumidity_bol(uint8_t *msg_u8p, mijaProcl_parsedData_t *outData_stp);
static bool ConvertBattery_bol(uint8_t *msg_u8p, mijaProcl_parsedData_t *outData_stp);
static bool ConvertTempHum_bol(uint8_t *msg_u8p, mijaProcl_parsedData_t *outData_stp);

/***************************************************************************************/
/* Local variables: */

/***************************************************************************************/
/* Global functions (unlimited visibility) */

/**--------------------------------------------------------------------------------------
 * @brief     parses a message string and sets the ouput data structure
*//*-----------------------------------------------------------------------------------*/
bool mijaProcl_ParseMessage_bol(uint8_t *msg_u8p, 
                                        mijaProcl_parsedData_t *outData_stp)
{
    bool exeResult_bol = false;

    if((NULL != msg_u8p) && (NULL != outData_stp))
    {
        // reset the data structure to initial values
        memset(outData_stp, 0U, sizeof(mijaProcl_parsedData_t));

        if(true == CheckTheMsgUuid_bol(msg_u8p, outData_stp))
        {
            ParseTheMsgCounter_vd(msg_u8p, outData_stp);

            ParseMacAddr_vd(msg_u8p, outData_stp);

            if(true == ParseDataType_bol(msg_u8p, outData_stp))
            {
                exeResult_bol = true;
            }
        }
    }

    return(exeResult_bol);

}

/***************************************************************************************/
/* Local functions: */

/**--------------------------------------------------------------------------------------
 * @brief     check for the expected UUID of the message and sets it in the out data
 * @author    S. Wink
 * @date      13. Jan. 2020
 * @param     msg_u8p       pointer to input data with the message 
 * @param     outData_stp   pointer to the ouput message data structure 
 * @return    true if UUID is as expected, else false
*//*-----------------------------------------------------------------------------------*/
static bool CheckTheMsgUuid_bol(uint8_t *msg_u8p, mijaProcl_parsedData_t *outData_stp)
{
    bool exeResult_bol = false;

    if(     (UUID_DATA_LOW_VAL == *(msg_u8p + UUID_DATA_LOW_ADR)) 
		&& (UUID_DATA_HIGH_VAL == *(msg_u8p + UUID_DATA_HIGH_ADR)))
	{
		outData_stp->uuid_u16 = *(msg_u8p + UUID_DATA_LOW_ADR);
        outData_stp->uuid_u16 = outData_stp->uuid_u16 << 8U;
        outData_stp->uuid_u16 += *(msg_u8p + UUID_DATA_HIGH_ADR); 
        exeResult_bol = true;
	}
    else
    {
        exeResult_bol = false;
        outData_stp->parseResult_u8 |= RET_CODE_ERROR_UUID_MISSMATCH;
    }
    
    return(exeResult_bol);
}

/**--------------------------------------------------------------------------------------
 * @brief     retrieves the message counter from the message, it is expected, that
 *            the privious checks are already valid (e.g. UUID)
 * @author    S. Wink
 * @date      13. Jan. 2020
 * @param     msg_u8p       pointer to input data with the message 
 * @param     outData_stp   pointer to the ouput message data structure 
*//*-----------------------------------------------------------------------------------*/
static void ParseTheMsgCounter_vd(uint8_t *msg_u8p, mijaProcl_parsedData_t *outData_stp)
{
    outData_stp->msgCnt_u8 = *(msg_u8p + MSG_CNT_ADR);
}

/**--------------------------------------------------------------------------------------
 * @brief     retrieves the MAC address of the device from the message, it is expected, 
 *            that the privious checks are already valid (e.g. UUID)
 * @author    S. Wink
 * @date      13. Jan. 2020
 * @param     msg_u8p       pointer to input data with the message 
 * @param     outData_stp   pointer to the ouput message data structure 
*//*-----------------------------------------------------------------------------------*/
static void ParseMacAddr_vd(uint8_t *msg_u8p, mijaProcl_parsedData_t *outData_stp)
{
    uint8_t msgIdx_u8 = DEVICE_MAC_ADR;
    uint8_t macIdx_u8 = 0U;
   
    while(6 != macIdx_u8)
    {
        outData_stp->macAddr_u8a[macIdx_u8] = *(msg_u8p + msgIdx_u8);
        msgIdx_u8--;
        macIdx_u8++;
    }
}

/**--------------------------------------------------------------------------------------
 * @brief     checks the correct data type of the message.
 * @author    S. Wink
 * @date      13. Jan. 2020
 * @param     msg_u8p       pointer to input data with the message 
 * @param     outData_stp   pointer to the ouput message data structure 
*//*-----------------------------------------------------------------------------------*/
static bool ParseDataType_bol(uint8_t *msg_u8p, mijaProcl_parsedData_t *outData_stp)
{
    bool exeResult_bol = true;
    uint8_t msgType_u8 = *(msg_u8p + DATA_TYPE_ID_ADR);

    switch(msgType_u8)
    {
        case DATA_TYPE_ID_TEMP:
            outData_stp->dataType_en = mija_TYPE_HUMIDITY;
            exeResult_bol &= ConvertTemperature_bol(msg_u8p, outData_stp);
            break;
        case DATA_TYPE_ID_HUM:
            outData_stp->dataType_en = mija_TYPE_HUMIDITY;
            exeResult_bol &= ConvertHumidity_bol(msg_u8p, outData_stp);
            break;
        case DATA_TYPE_ID_BATT:
            outData_stp->dataType_en = mija_TYPE_BATTERY;
            exeResult_bol &= ConvertBattery_bol(msg_u8p, outData_stp);
            break;
        case DATA_TYPE_ID_TEMPHUM:
            outData_stp->dataType_en = mija_TYPE_TEMPHUM;
            exeResult_bol &= ConvertTempHum_bol(msg_u8p, outData_stp);
            break;
        default:
            outData_stp->dataType_en = mija_TYPE_UNKNOWN;
            exeResult_bol = false;
            outData_stp->parseResult_u8 |= RET_CODE_ERROR_UNKNOWN_TYPE;
            break;
    }
    
    return(exeResult_bol);
}

/**--------------------------------------------------------------------------------------
 * @brief     converts the message data to temperature
 * @author    S. Wink
 * @date      13. Jan. 2020
 * @param     msg_u8p       pointer to input data with the message 
 * @param     outData_stp   pointer to the ouput message data structure 
*//*-----------------------------------------------------------------------------------*/
static bool ConvertTemperature_bol(uint8_t *msg_u8p, mijaProcl_parsedData_t *outData_stp)
{
    bool exeResult_bol = false;
    uint16_t dataRaw_u16 = 0U;

    if(DATA_LEN_TEMP_STD == *(msg_u8p + DATA_LEN_ADR))
    {
        dataRaw_u16 = *(msg_u8p + DATA_TEMP_HIGHBYTE_ADR);
		dataRaw_u16 = dataRaw_u16 << 8U;
		dataRaw_u16 += *(msg_u8p + DATA_TEMP_LOWBYTE_ADR);
		outData_stp->temperature_f32 = ((float)dataRaw_u16) / 10.0F;
        exeResult_bol = true;
    }
    else
    {
        outData_stp->parseResult_u8 |= RET_CODE_UNEXPECTED_TEMP_LENGTH;
    }
    

    return(exeResult_bol);
}

/**--------------------------------------------------------------------------------------
 * @brief     converts the message data to humidity
 * @author    S. Wink
 * @date      13. Jan. 2020
 * @param     msg_u8p       pointer to input data with the message 
 * @param     outData_stp   pointer to the ouput message data structure 
*//*-----------------------------------------------------------------------------------*/
static bool ConvertHumidity_bol(uint8_t *msg_u8p, mijaProcl_parsedData_t *outData_stp)
{
    bool exeResult_bol = false;
    uint16_t dataRaw_u16 = 0U;

    if(DATA_LEN_HUM_STD == *(msg_u8p + DATA_LEN_ADR))
    {
        dataRaw_u16 = *(msg_u8p + DATA_HUM_HIGHBYTE_ADR);
        dataRaw_u16 = dataRaw_u16 << 8U;
        dataRaw_u16 += *(msg_u8p + DATA_HUM_LOWBYTE_ADR);
        outData_stp->humidity_f32 = ((float)dataRaw_u16) / 10.0F;
        exeResult_bol = true;
    }
    else
    {
        outData_stp->parseResult_u8 |= RET_CODE_UNEXPECTED_HUM_LENGTH;
    }

    return(exeResult_bol);
}

/**--------------------------------------------------------------------------------------
 * @brief     converts the message data to battery capacity in percentage
 * @author    S. Wink
 * @date      13. Jan. 2020
 * @param     msg_u8p       pointer to input data with the message 
 * @param     outData_stp   pointer to the ouput message data structure 
*//*-----------------------------------------------------------------------------------*/
static bool ConvertBattery_bol(uint8_t *msg_u8p, mijaProcl_parsedData_t *outData_stp)
{
    bool exeResult_bol = false;
    uint16_t dataRaw_u16 = 0U;

    if(DATA_LEN_BATTERY_STD == *(msg_u8p + DATA_LEN_ADR))
    {
        dataRaw_u16 = *(msg_u8p + DATA_BATTERY_LOWBYTE_ADR);
		outData_stp->battery_f32 = ((float)dataRaw_u16);
        exeResult_bol = true;
    }
    else
    {
        outData_stp->parseResult_u8 |= RET_CODE_UNEXPECTED_BAT_LENGTH;
    }

    return(exeResult_bol);
}

/**--------------------------------------------------------------------------------------
 * @brief     converts the message data to temperture and humidity
 * @author    S. Wink
 * @date      13. Jan. 2020
 * @param     msg_u8p       pointer to input data with the message 
 * @param     outData_stp   pointer to the ouput message data structure 
*//*-----------------------------------------------------------------------------------*/
static bool ConvertTempHum_bol(uint8_t *msg_u8p, mijaProcl_parsedData_t *outData_stp)
{
    bool exeResult_bol = false;
    uint16_t dataRaw_u16 = 0U;

    if(DATA_LEN_TEMPHUM_STD == *(msg_u8p + DATA_LEN_ADR))
    {
		dataRaw_u16 = *(msg_u8p + DATA_HUMTEMP_TEMP_HIGHBYTE_ADR);
		dataRaw_u16 = dataRaw_u16 << 8U;
		dataRaw_u16 += *(msg_u8p + DATA_HUMTEMP_TEMP_LOWBYTE_ADR);
		outData_stp->temperature_f32 = ((float)dataRaw_u16) / 10.0F;

        dataRaw_u16 = *(msg_u8p + DATA_HUMTEMP_HUM_HIGHBYTE_ADR);
        dataRaw_u16 = dataRaw_u16 << 8U;
        dataRaw_u16 += *(msg_u8p + DATA_HUMTEMP_HUM_LOWBYTE_ADR);
        outData_stp->humidity_f32 = ((float)dataRaw_u16) / 10.0F;

        exeResult_bol = true;
    }
    else
    {
        outData_stp->parseResult_u8 |= RET_CODE_UNEXPECTED_TEMPHUM_LENGTH;
    }

    return(exeResult_bol);
}

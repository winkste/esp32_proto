/*****************************************************************************************
* FILENAME :        otaUpdate.h
*
* DESCRIPTION :
*       Header file for over the air update handling.
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
#ifndef OTAUPDATE_H_
#define OTAUPDATE_H_

/****************************************************************************************/
/* Imported header files: */
#include "esp_err.h"
#include "stdint.h"

/****************************************************************************************/
/* Global constant defines: */

/****************************************************************************************/
/* Global function like macro defines (to be avoided): */

/****************************************************************************************/
/* Global type definitions (enum (en), struct (st), union (un), typedef (tx): */
typedef struct otaUpdate_param_tag
{

}otaUpdate_param_t;

typedef struct otaUpdate_obj_tag *otaUpdate_objHdl_t;

/****************************************************************************************/
/* Global function definitions: */

/**---------------------------------------------------------------------------------------
 * @brief     Set the generic init parameter stucture to defaults
 * @author    S. Wink
 * @date      10. Mar. 2019
 * @param     param_stp           pointer to the module parameters
 * @return    In case of a null pointer ESP_ERR, else ESP_OK
*//*-----------------------------------------------------------------------------------*/
extern esp_err_t otaUpdate_InitializeParameter_td(otaUpdate_param_t *param_stp);

/**---------------------------------------------------------------------------------------
 * @brief     Module initialization function
 * @author    S. Wink
 * @date      10. Mar. 2019
 * @param     param_stp           pointer to the module parameters
 * @return    Memory error, null pointer exception (ESP_ERR), else ESP_OK
*//*-----------------------------------------------------------------------------------*/
extern esp_err_t otaUpdate_Initialize_td(otaUpdate_param_t *param_stp);

/**---------------------------------------------------------------------------------------
 * @brief     Call to check if an OTA update is ongoing.
 * @author    S. Wink
 * @date      10. Mar. 2019
 * @return    1 if update is ongoing, else 0
*//*-----------------------------------------------------------------------------------*/
extern uint8_t otaUpdate_InProgress_u8(void);

/**---------------------------------------------------------------------------------------
 * @brief     Start an OTA update.
 * @author    S. Wink
 * @date      10. Mar. 2019
 * @return    ES_OK if update was started, else ESP_FAIL
*//*-----------------------------------------------------------------------------------*/
extern esp_err_t otaUpdate_Begin_st(void);

/**---------------------------------------------------------------------------------------
 * @brief     Call this function to write up to 4 kBytes of hex data.
 * @author    S. Wink
 * @date      10. Mar. 2019
 * @param     data_u8p      pointer to the source data to write
 * @param     length_u16    length of data in bytes
 * @return    ES_OK if update was started, else ESP_FAIL
*//*-----------------------------------------------------------------------------------*/
extern esp_err_t otaUpdate_WriteData_st(uint8_t *data_u8p, uint16_t length_u16);

/**---------------------------------------------------------------------------------------
 * @brief     Finish the over the air Software update process
 * @author    S. Wink
 * @date      10. Mar. 2019
 * @return    ES_OK if update was started, else ESP_FAIL
*//*-----------------------------------------------------------------------------------*/
extern esp_err_t otaUpdate_Finish_st(void);

/****************************************************************************************/
/* Global data definitions: */

#endif /* OTAUPDATE_H_ */

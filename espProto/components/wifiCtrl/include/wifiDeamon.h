/*****************************************************************************************
* FILENAME :        wifiDeamon.h
*
* DESCRIPTION :
*      Header file for wifi control module
*
* AUTHOR :    Stephan Wink        CREATED ON :    24.01.2019
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
#ifndef WIFIDEAMON_H_
#define WIFIDEAMON_H_

/****************************************************************************************/
/* Imported header files: */

#include "wifiIf.h"

/****************************************************************************************/
/* Global constant defines: */

/****************************************************************************************/
/* Global function like macro defines (to be avoided): */

/****************************************************************************************/
/* Global type definitions (enum (en), struct (st), union (un), typedef (tx): */
typedef enum wifiDeamon_mode_tag
{
    wifiDeamon_MODE_AUTO,
    wifiDeamon_MODE_MAN_STA,
    wifiDeamon_MODE_MAN_AP
}wifiDeamon_mode_t;

typedef struct wifiDeamon_param_tag
{
    wifiDeamon_mode_t mode_en;
    wifiIf_eventCallB2_t eventCb_st;
    uint8_t ssid[wifiIf_SIZE_OF_SSID_VECTOR];           /**< SSID of ESP32 soft-AP */
    uint8_t password[wifiIf_SIZE_OF_PASSWORD];       /**< Password of ESP32 soft-AP */
    char *initKey_chp;
}wifiDeamon_param_t;

/****************************************************************************************/
/* Global function definitions: */

/**---------------------------------------------------------------------------------------
 * @brief     General initialization of wifiDeamon parameter. This function needs to be
 *              called before the initialization.
 * @author    S. Wink
 * @date      05. Sep. 2019
 * @param     param_stp     wifi parameter structure
 * @return                  ESP_OK if successful, else ESP_FAIL
*//*-----------------------------------------------------------------------------------*/
extern esp_err_t wifiDeamon_InitializeParam_st(wifiDeamon_param_t *param_stp);

/**---------------------------------------------------------------------------------------
 * @brief     General initialization of wifi deamon. This function has to be
 *              executed before a start of the deamon.
 * @author    S. Wink
 * @date      05. Sep. 2019
 * @param     param_stp     wifi parameter structure
 * @return                  ESP_OK if successful, else ESP_FAIL
*//*-----------------------------------------------------------------------------------*/
extern esp_err_t wifiDeamon_Initialize_st(wifiDeamon_param_t *param_stp);

/**---------------------------------------------------------------------------------------
 * @brief     Start function for wifi deamon.
 * @author    S. Wink
 * @date      05. Sep. 2019
 * @return    ESP_OK if successful, else ESP_FAIL
*//*-----------------------------------------------------------------------------------*/
extern esp_err_t wifiDeamon_Start_st(void);

/**---------------------------------------------------------------------------------------
 * @brief     Stop function for wifi deamon.
 * @author    S. Wink
 * @date      05. Sep. 2019
 * @return    ESP_OK if successful, else ESP_FAIL
*//*-----------------------------------------------------------------------------------*/
extern esp_err_t wifiDeamon_Stop_st(void);

/****************************************************************************************/
/* Global data definitions: */

#endif

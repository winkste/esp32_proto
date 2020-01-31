/*****************************************************************************************
* FILENAME :        mijaDevice.h
*
* SHORT DESCRIPTION:
*   Header file for mijaDevice module.
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
#ifndef MIJADEVICE_H
#define MIJADEVICE_H

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
#include "esp_log.h"
#include "esp_err.h"

/****************************************************************************************/
/* Global constant defines: */

/****************************************************************************************/
/* Global function like macro defines (to be avoided): */

/****************************************************************************************/
/* Global type definitions (enum (en), struct (st), union (un), typedef (tx): */

typedef struct mijaDevice_param_tag
{

}mijaDevice_param_t;

/****************************************************************************************/
/* Global function definitions: */

/**--------------------------------------------------------------------------------------
 * @brief     pre-configure the initialization parameter of the module
 * @author    S. Wink
 * @date      13. Jan. 2020
 * @param     param_stp             allocated pointer to the initialization parameters
 * @return    ESP_OK in case of success, else ESP_FAIL
*//*-----------------------------------------------------------------------------------*/
extern esp_err_t mijaDevice_InitializeParameter(mijaDevice_param_t *param_stp);

/**--------------------------------------------------------------------------------------
 * @brief     initialization of the mijaDevice module
 * @author    S. Wink
 * @date      13. Jan. 2020
 * @param     param_stp             allocated pointer to the initialization parameters
 * @return    ESP_OK in case of success, else ESP_FAIL
*//*-----------------------------------------------------------------------------------*/
extern esp_err_t mijaDevice_Initialize_st(mijaDevice_param_t *param_stp);

/**--------------------------------------------------------------------------------------
 * @brief     activation of the mijaDevice module
 * @author    S. Wink
 * @date      13. Jan. 2020
 * @return    ESP_OK in case of success, else ESP_FAIL
*//*-----------------------------------------------------------------------------------*/
extern esp_err_t mijaDevice_Activate_st(void);

/**--------------------------------------------------------------------------------------
 * @brief     activation of the mijaDevice module
 * @author    S. Wink
 * @date      13. Jan. 2020
 * @return    ESP_OK in case of success, else ESP_FAIL
*//*-----------------------------------------------------------------------------------*/
extern esp_err_t mijaDevice_Deactivate_st(void);

/****************************************************************************************/
/* Global data definitions: */

#ifdef __cplusplus
}
#endif

#endif //MIJADEVICE_H

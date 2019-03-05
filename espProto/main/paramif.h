/*****************************************************************************************
* FILENAME :        paramif.h
*
* DESCRIPTION :
*       Header file for non volatile parameter storage
*
* Date: 14. February 2019
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
#ifndef PARAMIF_H
#define PARAMIF_H

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************************/
/* Imported header files: */
#include "esp_err.h"

/****************************************************************************************/
/* Global constant defines: */

/****************************************************************************************/
/* Global function like macro defines (to be avoided): */

/****************************************************************************************/
/* Global type definitions (enum (en), struct (st), union (un), typedef (tx): */
 typedef struct paramif_param_tag
 {

 }paramif_param_t;

 typedef struct paramif_allocParam_tag
 {
     const char *nvsIdent_cp;
     uint16_t length_u16;
     uint8_t *defaults_u8p;
 }paramif_allocParam_t;

 typedef struct paramif_obj_tag *paramif_objHdl_t;

/****************************************************************************************/
/* Global function definitions: */

 /**---------------------------------------------------------------------------------------
  * @brief     Set the generic init parameter stucture to defaults
  * @author    S. Wink
  * @date      15. Feb. 2019
  * @param     param_stp           pointer to the module parameters
  * @return    In case of a null pointer ESP_ERR, else ESP_OK
 *//*-----------------------------------------------------------------------------------*/
 extern esp_err_t paramif_InitializeParameter_td(paramif_param_t *param_stp);

 /**---------------------------------------------------------------------------------------
  * @brief     Module initialization function
  * @author    S. Wink
  * @date      15. Feb. 2019
  * @param     param_stp           pointer to the module parameters
  * @return    Memory error, null pointer exception (ESP_ERR), else ESP_OK
 *//*-----------------------------------------------------------------------------------*/
 extern esp_err_t paramif_Initialize_td(paramif_param_t *param_stp);

 /**---------------------------------------------------------------------------------------
  * @brief     Activates the parameter handling module, now get/set/erase
  *                 of individual parameters is allowed
  * @author    S. Wink
  * @date      15. Feb. 2019
  * @return    ESP_ERR in case of memory issues or wrong module state, else ESP_OK
 *//*-----------------------------------------------------------------------------------*/
 extern esp_err_t paramif_Activate_td(void);

 /**---------------------------------------------------------------------------------------
  * @brief     Deactivates the parameter handling module, now overall memory activities
  *                 are enabled (erase all)
  * @author    S. Wink
  * @date      15. Feb. 2019
  * @return    ESP_ERR in case of memory issues or wrong module state, else ESP_OK
 *//*-----------------------------------------------------------------------------------*/
 extern esp_err_t paramif_DeActivate_td(void);

 /**---------------------------------------------------------------------------------------
  * @brief     Erases all parameters
  * @author    S. Wink
  * @date      15. Feb. 2019
  * @return    ESP_ERR in case of memory issues or wrong module state, else ESP_OK
 *//*-----------------------------------------------------------------------------------*/
 extern esp_err_t paramif_EraseAllParameter_td(void);

/**---------------------------------------------------------------------------------------
 * @brief     Set the parameter structure for one object to defaults
 * @author    S. Wink
 * @date      15. Feb. 2019
 * @param     param_stp           pointer to parameter structure
 * @return    In case of a null pointer ESP_ERR, else ESP_OK
*//*-----------------------------------------------------------------------------------*/
extern esp_err_t paramif_InitializeAllocParameter_td(paramif_allocParam_t *param_stp);

/**---------------------------------------------------------------------------------------
 * @brief     Allocates a new parameter set and returns a pointer to the parameter. This
 *              is only possible if the module is not activated.
 * @author    S. Wink
 * @date      15. Feb. 2019
 * @param     param_stp           pointer to parameter structure
 * @return    NULL in case of an error, else ESP_OK
*//*-----------------------------------------------------------------------------------*/
extern paramif_objHdl_t paramif_Allocate_stp(paramif_allocParam_t *param_stp);

/**---------------------------------------------------------------------------------------
 * @brief     Read parameter from storage device
 * @author    S. Wink
 * @date      15. Feb. 2019
 * @param     handle_xp parameter handle
 * @param     dest_u8p  destination address for data
 * @return    ESP_ERR or ESP_OK, depending on the read result
*//*-----------------------------------------------------------------------------------*/
extern esp_err_t paramif_Read_td(paramif_objHdl_t handle_xp, uint8_t *dest_u8p);

/**---------------------------------------------------------------------------------------
 * @brief     Get parameter length
 * @author    S. Wink
 * @date      15. Feb. 2019
 * @param     handle_xp parameter handle
 * @return    0 in case of an error, else length of parameter data
*//*-----------------------------------------------------------------------------------*/
extern uint16_t paramif_GetLength_u16(paramif_objHdl_t handle_xp, uint8_t *src_u8p);

/**---------------------------------------------------------------------------------------
 * @brief     writes data to the non volatile flash
 * @author    S. Wink
 * @date      15. Feb. 2019
 * @param     handle_xp parameter handle
 * @param     src_u8p   pointer to source destination which shall be transferred to nvs
 * @return    0 in case of an error, else length of parameter data
*//*-----------------------------------------------------------------------------------*/
extern esp_err_t paramif_Write_td(paramif_objHdl_t handle_xp, uint8_t *src_u8p);

/**---------------------------------------------------------------------------------------
 * @brief     erase the parameter to 0xFF
 * @author    S. Wink
 * @date      15. Feb. 2019
 * @param     handle_xp parameter handle
 * @return    ESP_ERR in case of an error, else ESP_OK
*//*-----------------------------------------------------------------------------------*/
extern esp_err_t paramif_Erase_td(paramif_objHdl_t handle_xp);

/**---------------------------------------------------------------------------------------
 * @brief     set the data to the default values
 * @author    S. Wink
 * @date      15. Feb. 2019
 * @param     handle_xp parameter handle
 * @return    ESP_ERR in case of an error, else ESP_OK
*//*-----------------------------------------------------------------------------------*/
extern esp_err_t paramif_ResetToDefault_td(paramif_objHdl_t handle_xp);

/**---------------------------------------------------------------------------------------
 * @brief     set the data to the default values
 * @author    S. Wink
 * @date      15. Feb. 2019
 * @param     handle_xp parameter handle
 * @return    ESP_ERR in case of an error, else ESP_OK
*//*-----------------------------------------------------------------------------------*/
extern void paramif_PrintHandle_vd(paramif_objHdl_t handle_xp);

/****************************************************************************************/
/* Global data definitions: */

#ifdef __cplusplus
}
#endif

#endif

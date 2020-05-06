/*****************************************************************************************
* FILENAME :        paramif.c
*
* DESCRIPTION :
*       This module handles the non volatile parameter storage
*
* AUTHOR :    Stephan Wink        CREATED ON :    14.02.2019
*
* PUBLIC FUNCTIONS :
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
*****************************************************************************************/

/****************************************************************************************/
/* Include Interfaces */

#include "paramif.h"

#include "nvs.h"
#include "nvs_flash.h"
#include "esp_err.h"
#include "esp_log.h"

#include <string.h>

/****************************************************************************************/
/* Local constant defines */
#define config_MAX_PARAMETER_OBJECTS    10
#define STORAGE_NAMESPACE "parameter"
static const char *TAG = "paramif";

/****************************************************************************************/
/* Local function like makros */

/****************************************************************************************/
/* Local type definitions (enum, struct, union) */

 typedef struct paramif_obj_tag
 {
     paramif_allocParam_t param_st;
     uint8_t objPoolIdx_u8;
 }paramif_obj_t;

 typedef enum moduleState_tag
 {
     STATE_NOT_INITIALIZED,
     STATE_INITIALIZED
 }moduleState_t;

/****************************************************************************************/
/* Local functions prototypes: */

/****************************************************************************************/
/* Local variables: */

 static paramif_obj_t objectPool_stsa[config_MAX_PARAMETER_OBJECTS];
 static moduleState_t moduleState_ens = STATE_NOT_INITIALIZED;
/****************************************************************************************/
/* Global functions (unlimited visibility) */

/**---------------------------------------------------------------------------------------
* @brief     Set the generic init parameter structure to defaults
* @author    S. Wink
* @date      15. Feb. 2019
* @param     param_stp           pointer to the module parameters
* @return    In case of a null pointer ESP_ERR, else ESP_OK
*//*-----------------------------------------------------------------------------------*/
esp_err_t paramif_InitializeParameter_td(paramif_param_t *param_stp)
{
    return(ESP_OK);
}

/**---------------------------------------------------------------------------------------
* @brief     Module initialization function
* @author    S. Wink
* @date      15. Feb. 2019
* @param     param_stp           pointer to the module parameters
* @return    Memory error, null pointer exception (ESP_ERR), else ESP_OK
*//*-----------------------------------------------------------------------------------*/
esp_err_t paramif_Initialize_td(paramif_param_t *param_stp)
{
    esp_err_t err_st = ESP_FAIL;
    uint8_t idx_u8 = 0U;


    if(STATE_NOT_INITIALIZED == moduleState_ens)
    {
        // deallocate all parameter objects from pool
        while(config_MAX_PARAMETER_OBJECTS > idx_u8)
        {
            objectPool_stsa[idx_u8].objPoolIdx_u8 = config_MAX_PARAMETER_OBJECTS;
            idx_u8++;
        }

        err_st = nvs_flash_init();
        if (   (ESP_ERR_NVS_NO_FREE_PAGES == err_st)
            || (ESP_ERR_NVS_NEW_VERSION_FOUND == err_st))
        {
            ESP_ERROR_CHECK(nvs_flash_erase());
            err_st = nvs_flash_init();
        }
        ESP_ERROR_CHECK(err_st);
    }
    else
    {
        err_st = ESP_FAIL;  // called initialize with the wrong state
    }

    if(ESP_OK == err_st)
    {
        moduleState_ens = STATE_INITIALIZED;
    }

    return(err_st);
}

/**---------------------------------------------------------------------------------------
* @brief     Activates the parameter handling module, now get/set/erase
*                 of individual parameters is allowed
* @author    S. Wink
* @date      15. Feb. 2019
* @return    ESP_ERR in case of memory issues or wrong module state, else ESP_OK
*//*-----------------------------------------------------------------------------------*/
esp_err_t paramif_Activate_td(void)
{
    // obsolete???
    return(ESP_FAIL);
}

/**---------------------------------------------------------------------------------------
* @brief     Deactivates the parameter handling module, now overall memory activities
*                 are enabled (erase all)
* @author    S. Wink
* @date      15. Feb. 2019
* @return    ESP_ERR in case of memory issues or wrong module state, else ESP_OK
*//*-----------------------------------------------------------------------------------*/
esp_err_t paramif_DeActivate_td(void)
{
    //obsolete??
    return(ESP_FAIL);
}

/**---------------------------------------------------------------------------------------
* @brief     Erases all parameters
* @author    S. Wink
* @date      15. Feb. 2019
* @return    ESP_ERR in case of memory issues or wrong module state, else ESP_OK
*//*-----------------------------------------------------------------------------------*/
esp_err_t paramif_EraseAllParameter_td(void)
{
    esp_err_t err_st;

    ESP_ERROR_CHECK(nvs_flash_erase());
    err_st = nvs_flash_init();
    ESP_ERROR_CHECK(err_st);

    return(err_st);
}

/**---------------------------------------------------------------------------------------
 * @brief     Set the parameter structure for one object to defaults
 * @author    S. Wink
 * @date      15. Feb. 2019
 * @param     param_stp           pointer to parameter structure
 * @return    In case of a null pointer ESP_ERR, else ESP_OK
*//*-----------------------------------------------------------------------------------*/
esp_err_t paramif_InitializeAllocParameter_td(paramif_allocParam_t *param_stp)
{
    esp_err_t err_st = ESP_FAIL;
    if((NULL != param_stp) && (STATE_INITIALIZED == moduleState_ens))
    {
        param_stp->defaults_u8p = NULL;
        param_stp->length_u16 = 0U;
        param_stp->nvsIdent_cp = NULL;
        err_st = ESP_OK;
    }
    else
    {
        err_st = ESP_FAIL;
    }

    return(err_st);
}

/**---------------------------------------------------------------------------------------
 * @brief     Allocates a new parameter set and returns a pointer to the parameter. This
 *              is only possible if the module is not activated.
 * @author    S. Wink
 * @date      15. Feb. 2019
 * @param     param_stp           pointer to parameter structure
 * @return    NULL in case of an error, else ESP_OK
*//*-----------------------------------------------------------------------------------*/
paramif_objHdl_t paramif_Allocate_stp(paramif_allocParam_t *param_stp)
{
    paramif_objHdl_t retObj_xp = NULL;
    uint8_t idx_u8 = 0U;


    if((NULL != param_stp) && (STATE_INITIALIZED == moduleState_ens))
    {
        while(config_MAX_PARAMETER_OBJECTS > idx_u8)
        {
            if(config_MAX_PARAMETER_OBJECTS == objectPool_stsa[idx_u8].objPoolIdx_u8)
            {
                // we found an empty object in the pool, use it
                objectPool_stsa[idx_u8].objPoolIdx_u8 = idx_u8;
                memcpy(&objectPool_stsa[idx_u8].param_st, param_stp,
                        sizeof(objectPool_stsa[idx_u8].param_st));
                retObj_xp = &objectPool_stsa[idx_u8];
                break;
            }
            idx_u8++;
        }
    }
    return(retObj_xp);
}

/**---------------------------------------------------------------------------------------
 * @brief     De-Allocates a new parameter set.
 * @author    S. Wink
 * @date      15. Feb. 2019
 * @param     paraObj_xp           pointer to parameter object handle
 * @return    NULL in case of an error, else ESP_OK
*//*-----------------------------------------------------------------------------------*/
void paramif_DeAllocate_stp(paramif_objHdl_t paraObj_xp)
{
    if(NULL != paraObj_xp)
    {
        paraObj_xp->objPoolIdx_u8 = config_MAX_PARAMETER_OBJECTS;
    }
}

/**---------------------------------------------------------------------------------------
 * @brief     Read parameter from storage device
 * @author    S. Wink
 * @date      15. Feb. 2019
 * @param     handle_xp parameter handle
 * @param     dest_u8p  destination address for data
 * @return    ESP_ERR or ESP_OK, depending on the read result
*//*-----------------------------------------------------------------------------------*/
esp_err_t paramif_Read_td(paramif_objHdl_t handle_xp, uint8_t *dest_u8p)
{
    esp_err_t err_st = ESP_FAIL;
    nvs_handle fileHandle_st;
    uint16_t length_u16 = 0U;

    if((NULL != handle_xp) && (STATE_INITIALIZED == moduleState_ens))
    {
        length_u16 = handle_xp->param_st.length_u16;
        err_st = nvs_open(STORAGE_NAMESPACE, NVS_READONLY, &fileHandle_st);

        if(ESP_OK == err_st)
        {
            err_st = nvs_get_blob(fileHandle_st,
                                    handle_xp->param_st.nvsIdent_cp, dest_u8p,
                                    (size_t *)&length_u16);

            if(ESP_OK == err_st)
            {
                nvs_close(fileHandle_st);
            }
        }

        /* if everything works fine, the error return status should stay at
         * ESP_OK if this is not the case, we will set the data to defaults if
         * available */
        if((ESP_OK != err_st) && handle_xp->param_st.defaults_u8p != NULL)
        {
            err_st = ESP_OK;
            memcpy(dest_u8p, handle_xp->param_st.defaults_u8p,
                    handle_xp->param_st.length_u16);
            err_st = paramif_Write_td(handle_xp, handle_xp->param_st.defaults_u8p);
        }
    }

    return(err_st);
}

/**---------------------------------------------------------------------------------------
 * @brief     Get parameter length
 * @author    S. Wink
 * @date      15. Feb. 2019
 * @param     handle_xp parameter handle
 * @return    0 in case of an error, else length of parameter data
*//*-----------------------------------------------------------------------------------*/
uint16_t paramif_GetLength_u16(paramif_objHdl_t handle_xp, uint8_t *src_u8p)
{
    uint16_t length_u16 = 0U;

    if((NULL != handle_xp) && (STATE_INITIALIZED == moduleState_ens))
    {
        length_u16 = handle_xp->param_st.length_u16;
    }

    return(length_u16);
}

/**---------------------------------------------------------------------------------------
 * @brief     writes data to the non volatile flash
 * @author    S. Wink
 * @date      15. Feb. 2019
 * @param     handle_xp parameter handle
 * @param     src_u8p   pointer to source destination which shall be transferred to nvs
 * @return    0 in case of an error, else length of parameter data
*//*-----------------------------------------------------------------------------------*/
esp_err_t paramif_Write_td(paramif_objHdl_t handle_xp, uint8_t *src_u8p)
{
    esp_err_t err_st = ESP_FAIL;
    nvs_handle fileHandle_st;

    if((NULL != handle_xp) && (STATE_INITIALIZED == moduleState_ens))
    {
        err_st = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &fileHandle_st);

        if(ESP_OK == err_st)
        {
            err_st = nvs_set_blob(fileHandle_st, handle_xp->param_st.nvsIdent_cp,
                    src_u8p, handle_xp->param_st.length_u16);
            if(ESP_OK == err_st)
            {
                err_st = nvs_commit(fileHandle_st);
            }
            nvs_close(fileHandle_st);
        }
    }

    return(err_st);
}

/**---------------------------------------------------------------------------------------
 * @brief     erase the parameter to 0xFF
 * @author    S. Wink
 * @date      15. Feb. 2019
 * @param     handle_xp parameter handle
 * @return    ESP_ERR in case of an error, else ESP_OK
*//*-----------------------------------------------------------------------------------*/
esp_err_t paramif_Erase_td(paramif_objHdl_t handle_xp)
{
    return(ESP_FAIL);
}

/**---------------------------------------------------------------------------------------
 * @brief     set the data to the default values
 * @author    S. Wink
 * @date      15. Feb. 2019
 * @param     handle_xp parameter handle
 * @return    ESP_ERR in case of an error, else ESP_OK
*//*-----------------------------------------------------------------------------------*/
esp_err_t paramif_ResetToDefault_td(paramif_objHdl_t handle_xp)
{
    esp_err_t err_st = ESP_FAIL;

    if(   (NULL != handle_xp)
       && (STATE_INITIALIZED == moduleState_ens)
       && (NULL != handle_xp->param_st.defaults_u8p))
    {
        err_st = paramif_Write_td(handle_xp, handle_xp->param_st.defaults_u8p);
    }
    else
    {
        err_st = ESP_FAIL;
    }
    return(err_st);
}

/**---------------------------------------------------------------------------------------
 * @brief     Prints Handle information
 * @author    S. Wink
 * @date      15. Feb. 2019
 * @param     handle_xp parameter handle
 * @return    none
*//*-----------------------------------------------------------------------------------*/
void paramif_PrintHandle_vd(paramif_objHdl_t handle_xp)
{
    ESP_LOGI(TAG, "---- parameter information request ----");
    if(NULL == handle_xp)
    {
        ESP_LOGI(TAG, "handle is set to null");
    }
    else
    {
        ESP_LOGI(TAG, "index of object: %d", handle_xp->objPoolIdx_u8);
        ESP_LOGI(TAG, "parameter id: %s", handle_xp->param_st.nvsIdent_cp);
        ESP_LOGI(TAG, "length of parameter: %d", handle_xp->param_st.length_u16);
        if(NULL == handle_xp->param_st.defaults_u8p)
        {
            ESP_LOGI(TAG, "parameter defaults are null");
        }
    }
}

/****************************************************************************************/
/* Local functions: */



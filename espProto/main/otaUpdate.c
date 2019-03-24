/*****************************************************************************************
* FILENAME :        otaUpdate.c
*
* DESCRIPTION :
*       This module
*
* AUTHOR :    Stephan Wink        CREATED ON :    10.03.2019
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
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
*****************************************************************************************/

/****************************************************************************************/
/* Include Interfaces */

#include "otaUpdate.h"

#include "esp_err.h"
#include "esp_partition.h"
#include "esp_ota_ops.h"
#include "esp_spi_flash.h"
#include "esp_log.h"
#include "string.h"

#include "argtable3/argtable3.h"
#include "myConsole.h"

/****************************************************************************************/
/* Local constant defines */

#define OTA_MESSAGE_WRITE_LENGTH    4096U

/****************************************************************************************/
/* Local function like makros */

/****************************************************************************************/
/* Local type definitions (enum, struct, union) */
 typedef enum moduleState_tag
 {
     STATE_NOT_INITIALIZED,
     STATE_INITIALIZED,
     STATE_UPDATE_IN_PROGRESS
 }moduleState_t;

/****************************************************************************************/
/* Local functions prototypes: */
static const esp_partition_t * FindNextBootPartition_stc(void);
static void otaUpdate_RegisterCommands(void);
static int OtaCommandHandler_i(int argc, char** argv);

/****************************************************************************************/
/* Local variables: */

// Partition for the OTA update.
static const esp_partition_t *otaPartition_stsc;

// SPI flash address for next write operation.
static uint32_t currentFlashAddr_u32s;

static esp_ota_handle_t otaHandle_sts;

static moduleState_t moduleState_ens = STATE_NOT_INITIALIZED;

static const char * TAG = "otaUpdate";

/** Arguments used by 'parameter' function */
static struct
{
    struct arg_str *cmd_stp;
    struct arg_int *dLength_stp;
    struct arg_int *seqNo_stp;
    struct arg_str *data_stp;
    struct arg_end *end_stp;
}otaCmdArgs_sts;

static const int32_t CMD_EXE_SUCCESS  = 0;
static const int32_t CMD_EXE_FAIL     = 1;
static const uint8_t CHARS_PER_BYTE   = 2U;

/****************************************************************************************/
/* Global functions (unlimited visibility) */

/**---------------------------------------------------------------------------------------
 * @brief     Set the generic init parameter stucture to defaults
*//*------------------------------------------------------------------------------------*/
esp_err_t otaUpdate_InitializeParameter_td(otaUpdate_param_t *param_stp)
{
    moduleState_ens = STATE_NOT_INITIALIZED;
    return(ESP_OK);
}

/**---------------------------------------------------------------------------------------
 * @brief     Module initialization function
*//*------------------------------------------------------------------------------------*/
esp_err_t otaUpdate_Initialize_td(otaUpdate_param_t *param_stp)
{
    spi_flash_init();
    moduleState_ens = STATE_INITIALIZED;
    otaUpdate_RegisterCommands();

    return(ESP_OK);
}

/**---------------------------------------------------------------------------------------
 * @brief     Call to check if an OTA update is ongoing.
*//*------------------------------------------------------------------------------------*/
uint8_t otaUpdate_InProgress_u8(void)
{
    return(STATE_UPDATE_IN_PROGRESS == moduleState_ens);
}

/**---------------------------------------------------------------------------------------
 * @brief     Start an OTA update.
*//*------------------------------------------------------------------------------------*/
extern esp_err_t otaUpdate_Begin_st(void)
{
    esp_err_t retVal_st = ESP_FAIL;

    if(STATE_INITIALIZED == moduleState_ens)
    {
        otaPartition_stsc = FindNextBootPartition_stc();
        if (NULL == otaPartition_stsc)
        {
            retVal_st = ESP_FAIL;
        }
        else
        {
            currentFlashAddr_u32s = otaPartition_stsc->address;
            ESP_LOGI(TAG, "Set start address for flash writes to 0x%08x",
                        currentFlashAddr_u32s);

            retVal_st = esp_ota_begin(otaPartition_stsc, 4096, &otaHandle_sts);
            ESP_LOGI(TAG, "Result from esp_ota_begin: %d %d", retVal_st, otaHandle_sts);
        }
    }

    if(ESP_OK == retVal_st)
    {
        moduleState_ens = STATE_UPDATE_IN_PROGRESS;
    }

    return(retVal_st);
}

/**---------------------------------------------------------------------------------------
 * @brief     Call this function to write up to 4 kBytes of hex data.
*//*------------------------------------------------------------------------------------*/
extern esp_err_t otaUpdate_WriteData_st(uint8_t *data_u8p, uint16_t length_u16)
{
    uint16_t flashSectorToErase_u16 = 0U;
    esp_err_t err_st = ESP_FAIL;

    if(STATE_UPDATE_IN_PROGRESS == moduleState_ens)
    {
        err_st = ESP_OK;
    }

    if((ESP_OK == err_st) && OTA_MESSAGE_WRITE_LENGTH > length_u16)
    {
        // Erase flash pages at 4k boundaries.
        if (currentFlashAddr_u32s % 0x1000 == 0)
        {
            flashSectorToErase_u16 = currentFlashAddr_u32s / 0x1000U;
            ESP_LOGI("ota_update", "Erasing flash sector %d", flashSectorToErase_u16);
            spi_flash_erase_sector(flashSectorToErase_u16);
        }

        // Write data into flash memory.
        ESP_LOGI(TAG, "Writing flash at 0x%08x...", currentFlashAddr_u32s);
        err_st = esp_ota_write(otaHandle_sts, data_u8p, length_u16);
        if (ESP_OK != err_st)
        {
            ESP_LOGE(TAG, "Failed to write flash at address 0x%08x, error %d",
                    currentFlashAddr_u32s, err_st);
        }

        currentFlashAddr_u32s += length_u16;
    }

    return(ESP_OK);
}

/**---------------------------------------------------------------------------------------
 * @brief     Finish the over the air Software update process
 * @author    S. Wink
 * @date      10. Mar. 2019
 * @return    ES_OK if update was started, else ESP_FAIL
*//*------------------------------------------------------------------------------------*/
extern esp_err_t otaUpdate_Finish_st(void)
{
    esp_err_t retVal_st = ESP_OK;

    if(STATE_UPDATE_IN_PROGRESS == moduleState_ens)
    {
        ESP_LOGI(TAG, "Finish in progress");
        if (!otaPartition_stsc)
        {
            retVal_st = ESP_FAIL;
            ESP_LOGI(TAG, "Finish partition error");
        }

        if(ESP_OK == retVal_st)
        {
            retVal_st = esp_ota_end(otaHandle_sts);
            ESP_LOGI(TAG, "Finish esp ota end return: %d", retVal_st);
        }

        if(ESP_OK == retVal_st)
        {
            retVal_st = esp_ota_set_boot_partition(otaPartition_stsc);
            ESP_LOGI(TAG, "Finish boot partition set: %d", retVal_st);

            if(ESP_OK == retVal_st)
            {
                ESP_LOGI(TAG, "Boot partition activated: %s",
                            otaPartition_stsc->label);
            }
            else
            {
                otaPartition_stsc = NULL;
                ESP_LOGE(TAG, "Failed to activate boot partition %s, error %d",
                        otaPartition_stsc->label, retVal_st);
            }
        }

        // independent from the result, set the status back to initialized to
        // enable the next update, if needed
        moduleState_ens = STATE_INITIALIZED;
    }

    return(retVal_st);
}
/****************************************************************************************/
/* Local functions: */

/**---------------------------------------------------------------------------------------
 * @brief     Searches for the next available boot partition
 *              Factory -> OTA_0
 *              OTA_0   -> OTA_1
 *              OTA_1   -> OTA_0
 * @author    S. Wink
 * @date      10. Mar. 2019
 * @return    pointer to boot partition object or NULL
*//*-----------------------------------------------------------------------------------*/
static const esp_partition_t * FindNextBootPartition_stc(void)
{
    const esp_partition_t *current_stp = esp_ota_get_boot_partition();
    const esp_partition_t *next_stp = NULL;

    if (!strcmp("factory", current_stp->label))
    {
        next_stp = esp_partition_find_first(ESP_PARTITION_TYPE_APP,
                                                        ESP_PARTITION_SUBTYPE_ANY,
                                                        "ota_0");
    }else if (!strcmp("ota_0", current_stp->label))
    {
        next_stp = esp_partition_find_first(ESP_PARTITION_TYPE_APP,
                                                        ESP_PARTITION_SUBTYPE_ANY,
                                                        "ota_1");
    }else if (!strcmp("ota_1", current_stp->label))
    {
        next_stp = esp_partition_find_first(ESP_PARTITION_TYPE_APP,
                                                        ESP_PARTITION_SUBTYPE_ANY,
                                                        "ota_0");
    }
    else
    {
        ESP_LOGE(TAG, "Failed to find next boot partition from current boot partition: %s",
                        current_stp ? current_stp->label : "NULL");
    }

    if (next_stp)
    {
        ESP_LOGI(TAG, "Found next boot partition: %02x %02x 0x%08x %s",
                next_stp->type, next_stp->subtype,
                next_stp->address, next_stp->label);
    }

    return(next_stp);
}

/**---------------------------------------------------------------------------------------
 * @brief     Function to initialize OTA commands
 * @author    S. Wink
 * @date      24. Jan. 2019
*//*-----------------------------------------------------------------------------------*/
static void otaUpdate_RegisterCommands(void)
{
    otaCmdArgs_sts.cmd_stp = arg_str1(NULL, NULL, "<cmd>", "OTA control command");
    otaCmdArgs_sts.dLength_stp = arg_int0("l", "length", "<i>", "length of bytes");
    otaCmdArgs_sts.dLength_stp->ival[0] = 0U; // set default value
    otaCmdArgs_sts.seqNo_stp = arg_int0("s", "seqno", "<i>", "sequence no of data package");
    otaCmdArgs_sts.seqNo_stp->ival[0] = 0U; // set default value
    otaCmdArgs_sts.data_stp = arg_str0("d", "data", "<data>", "data vector");
    otaCmdArgs_sts.end_stp = arg_end(3);

    const myConsole_cmd_t paramCmd =
    {
        .command = "ota",
        .help = "OTA command control",
        .hint = NULL,
        .func = &OtaCommandHandler_i,
        .argtable = &otaCmdArgs_sts
    };

    ESP_ERROR_CHECK(myConsole_CmdRegister_td(&paramCmd));
}

/**---------------------------------------------------------------------------------------
 * @brief     Handler for console command for OTA control
 * @author    S. Wink
 * @date      24. Jan. 2019
 * @param     argc  count of argument list
 * @param     argv  pointer to argument list
 * @return    not equal to zero if error detected
*//*-----------------------------------------------------------------------------------*/
static int OtaCommandHandler_i(int argc, char** argv)
{
    int32_t cmdExeResult_s32 = CMD_EXE_FAIL;
    esp_err_t err_st = ESP_FAIL;
    int32_t nerrors_s32 = arg_parse(argc, argv, (void**) &otaCmdArgs_sts);

    if (nerrors_s32 != 0)
    {
        arg_print_errors(stderr, otaCmdArgs_sts.end_stp, argv[0]);
        return(cmdExeResult_s32);
    }

    if('b' == **otaCmdArgs_sts.cmd_stp->sval)
    {
        ESP_LOGI(TAG, "firmware update begin received...");

        err_st = otaUpdate_Begin_st();

        if(ESP_OK == err_st)
        {
            cmdExeResult_s32 = CMD_EXE_SUCCESS;
        }

    }
    else if('w' == **otaCmdArgs_sts.cmd_stp->sval)
    {
        ESP_LOGI(TAG, "firmware update write received...");
        if(   (0 != otaCmdArgs_sts.dLength_stp->count)
           && (NULL != otaCmdArgs_sts.dLength_stp->ival)
           && (0 != otaCmdArgs_sts.data_stp->count)
           && (NULL != otaCmdArgs_sts.data_stp->sval))
        {
            uint16_t length_u16 = *otaCmdArgs_sts.dLength_stp->ival;
            uint8_t *data_u8p;
            const char *source_chp = *otaCmdArgs_sts.data_stp->sval;

            // we expect here a format of two character = one byte hex
            // so the length of the byte array is length of the input array
            // divided by 2, this also means only even length values are valid
            if(0U == (length_u16 % CHARS_PER_BYTE))
            {
                length_u16 = length_u16 / CHARS_PER_BYTE;
                data_u8p = malloc(length_u16 * sizeof(uint8_t));

                if(NULL != data_u8p)
                {
                    uint32_t conv_u32;
                    for (uint16_t idx_u16 = 0U; idx_u16 < length_u16; idx_u16++)
                    {
                        // convert the string back to byte array: "E5" -> 0xE5
                        sscanf(source_chp, "%02x", &conv_u32);
                        *(data_u8p + idx_u16) = (uint8_t)conv_u32;
                        source_chp += CHARS_PER_BYTE; // two characters = one byte
                    }

                    err_st = otaUpdate_WriteData_st(data_u8p, length_u16);
                    if(ESP_OK == err_st)
                    {
                        cmdExeResult_s32 = CMD_EXE_SUCCESS;
                    }

                    free(data_u8p);
                }
            }
        }
    }
    else if('e' == **otaCmdArgs_sts.cmd_stp->sval)
    {
        ESP_LOGI(TAG, "firmware update end received...");

        err_st = otaUpdate_Finish_st();

        if(ESP_OK == err_st)
        {
            cmdExeResult_s32 = CMD_EXE_SUCCESS;
        }
    }
    else
    {
        ESP_LOGI(TAG, "firmware update unrecognized ota cmd...");
        cmdExeResult_s32 = CMD_EXE_FAIL;
    }

    return(cmdExeResult_s32);
}


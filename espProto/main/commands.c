/*
 * commands.c
 *
 *  Created on: 26 Jan 2019
 *      Author: stephan_wink
 */

#include "commands.h"

#include "sdkconfig.h"
#include "stdint.h"
#include "string.h"
#include "esp_err.h"
#include "esp_log.h"

#define MAX_SUPPORTED_COMMANDS 10

typedef struct moduleData_tag
{
    cmdElement_t cmdVector_sta[MAX_SUPPORTED_COMMANDS];
    uint8_t commands_u8;
}moduleData_t;

static const char *TAG = "commands";
static moduleData_t data_sts;

void commands_Initialize(void)
{
    memset(&data_sts, 0U, sizeof(data_sts));
    data_sts.commands_u8 = 0U;
    ESP_LOGI(TAG, "commands_Initialize...");
}

esp_err_t commands_AddElement(cmdElement_t *cmdElem_pst)
{
    esp_err_t retCode_st = ESP_OK;

    if(MAX_SUPPORTED_COMMANDS > data_sts.commands_u8)
    {
        memcpy(&data_sts.cmdVector_sta[data_sts.commands_u8], cmdElem_pst,
                sizeof(data_sts.cmdVector_sta[data_sts.commands_u8]));
        data_sts.commands_u8++;
        ESP_LOGI(TAG, "add command %s ...", cmdElem_pst->cmd_ccp);
        retCode_st = ESP_OK;
    }
    else
    {
        ESP_LOGE(TAG, "out of memory for commands");
        retCode_st = ESP_ERR_NO_MEM;
    }
    return(retCode_st);
}

void commands_Execute(char *buffer_cp, uint16_t length_u16)
{
    uint8_t idx_u8 = 0U;


    for(idx_u8 = 0U; idx_u8 < data_sts.commands_u8; idx_u8++)
    {
        if(0 == strncmp(data_sts.cmdVector_sta[idx_u8].cmd_ccp,
                buffer_cp, data_sts.cmdVector_sta[idx_u8].cmdLength_u8))
        {
            ESP_LOGI(TAG, "command: %s, parameter: %s",
                      data_sts.cmdVector_sta[idx_u8].cmd_ccp,
                      &buffer_cp[data_sts.cmdVector_sta[idx_u8].cmdLength_u8]);
            if(NULL != data_sts.cmdVector_sta[idx_u8].commandCallBack_ptrs)
            {
                (*data_sts.cmdVector_sta[idx_u8].commandCallBack_ptrs)(
                          &buffer_cp[data_sts.cmdVector_sta[idx_u8].cmdLength_u8]);
            }
            ESP_LOGI(TAG, "executed ...");
            break;
        }
    }
}


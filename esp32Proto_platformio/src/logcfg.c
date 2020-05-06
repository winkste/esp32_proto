/****************************************************************************************
* FILENAME :        logcfg.c
*
* SHORT DESCRIPTION:
*   This module configures the project specific logging level of dedicated
*   modules. Use this configuration to determine the level of all your log setup.
*
* DETAILED DESCRIPTION :     
*
* AUTHOR :    Stephan Wink        CREATED ON :    31. Mar. 2020
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

#include "logcfg.h"

#include "stdint.h"
#include "stdbool.h"
#include "stddef.h"
#include "string.h"
#include "esp_log.h"
#include "esp_err.h"

/***************************************************************************************/
/* Local constant defines */

/***************************************************************************************/
/* Local function like makros */

/***************************************************************************************/
/* Local type definitions (enum, struct, union) */

/***************************************************************************************/
/* Local functions prototypes: */
static void SetLevelToDevault(void);

/***************************************************************************************/
/* Local variables: */

/***************************************************************************************/
/* Global functions (unlimited visibility) */

/**--------------------------------------------------------------------------------------
 * @brief     Starts the basic module
 * @author    S. Wink
 * @date      13. Jan. 2020
*//*-----------------------------------------------------------------------------------*/
esp_err_t logcfg_Configure_st(logcfg_logModuls_t cfg_en)
{
    esp_err_t exeResult_st = ESP_OK;

    SetLevelToDevault();
    switch(cfg_en)
    {
        case logcfg_WIFI:
            esp_log_level_set("phy_init", ESP_LOG_DEBUG);
            esp_log_level_set("wifi", ESP_LOG_DEBUG);
            esp_log_level_set("tcpip_adapter", ESP_LOG_DEBUG);
            esp_log_level_set("wifiStation", ESP_LOG_DEBUG);
            esp_log_level_set("wifiAp", ESP_LOG_DEBUG);
            esp_log_level_set("wifiCtrl", ESP_LOG_DEBUG);
            esp_log_level_set("udplog", ESP_LOG_DEBUG);
            break;
        case logcfg_DEVICES:
            esp_log_level_set("gendev", ESP_LOG_DEBUG);
            esp_log_level_set("mijasens", ESP_LOG_DEBUG);
            esp_log_level_set("mqttdrv", ESP_LOG_DEBUG);
            break;
        case logcfg_MQTT:
            esp_log_level_set("mqttdrv", ESP_LOG_DEBUG);
            break;
        case logcfg_ALL:
            break;
        default:
            exeResult_st = ESP_FAIL;
            break;
    }

    return(exeResult_st);

}

/***************************************************************************************/
/* Local functions: */
/**---------------------------------------------------------------------------------------
 * @brief   Set all levels first to default
 * @author  S. Wink
 * @date    01. Apr. 2020
 * @return  none
*//*------------------------------------------------------------------------------------*/
static void SetLevelToDevault(void)
{
    /*
    ESP_LOG_NONE ==     No log output 
    ESP_LOG_ERROR ==     Critical errors, software module can not recover on its own 
    ESP_LOG_WARN ==     Error conditions from which recovery measures have been taken 
    ESP_LOG_INFO ==     Information messages which describe normal flow of events 
    ESP_LOG_DEBUG ==     Extra information which is not necessary for normal use (values, 
                            pointers, sizes, etc). 
    ESP_LOG_VERBOSE ==     Bigger chunks of debugging information, or frequent messages 
                            which can potentially flood the output. 
    */
    esp_log_level_t level_en = ESP_LOG_WARN;


    esp_log_level_set("efuse", level_en);
    esp_log_level_set("gendev", level_en);
    esp_log_level_set("mqttdrv", level_en);
    esp_log_level_set("MQTT_CLIENT", level_en);
    esp_log_level_set("OUTBOX", level_en);
    esp_log_level_set("mijasens", level_en);
    esp_log_level_set("phy_init", level_en);
    esp_log_level_set("wifi", level_en);
    esp_log_level_set("tcpip_adapter", level_en);
    esp_log_level_set("wifiStation", level_en);
    esp_log_level_set("wifiAp", level_en);
    esp_log_level_set("wifiCtrl", level_en);
    esp_log_level_set("consoleSocket", level_en);
    esp_log_level_set("paramif", level_en);
    esp_log_level_set("nvs", level_en);
    esp_log_level_set("BTDM_INIT", level_en);
    esp_log_level_set("udpLog", level_en);
    esp_log_level_set("RTC_MODULE", level_en);
    esp_log_level_set("event", level_en);
    esp_log_level_set("TRANS_TCP", level_en);
}

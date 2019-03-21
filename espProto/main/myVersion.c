/*****************************************************************************************
* FILENAME :        myVersion.c
*
* DESCRIPTION :
*       This module
*
* AUTHOR :    Stephan Wink        CREATED ON :    08.03.2019
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
* REF NO  VERSION DATE    WHO     DETAIL
* 000       08.03         SWI     initial revisioned version
*****************************************************************************************/

/****************************************************************************************/
/* Include Interfaces */

#include"myVersion.h"

#include "esp_log.h"
#include "esp_err.h"

#include"myConsole.h"


/****************************************************************************************/
/* Local constant defines */

/****************************************************************************************/
/* Local function like makros */

/****************************************************************************************/
/* Local type definitions (enum, struct, union) */

/****************************************************************************************/
/* Local functions prototypes: */
static void RegisterCommands(void);
static int cmdVersionHandler_i(int argc, char** argv);

/****************************************************************************************/
/* Local variables: */

static const char *TAG              = "myVersion";
static const char *FW_IDENTIFIER    = "00005FW"; // Firmware identification
static const char *FW_VERSION       = "000";     // Firmware Version
static const char *FW_DESCRIPTION   = "prototype firmware for esp32";

/****************************************************************************************/
/* Global functions (unlimited visibility) */

/**---------------------------------------------------------------------------------------
 * @brief     Initialization function for myVersion
*//*-----------------------------------------------------------------------------------*/
esp_err_t myVersion_Initialize_st(void)
{
    RegisterCommands();
    return(ESP_OK);
}

/****************************************************************************************/
/* Local functions: */

/**---------------------------------------------------------------------------------------
 * @brief     Registration of the supported console commands
 * @author    S. Wink
 * @date      24. Jan. 2019
*//*-----------------------------------------------------------------------------------*/
static void RegisterCommands(void)
{
        const myConsole_cmd_t versionCmd = {
        .command = "ver",
        .help = "Get version information",
        .func = &cmdVersionHandler_i,
    };

    ESP_ERROR_CHECK(myConsole_CmdRegister_td(&versionCmd));
}

/**---------------------------------------------------------------------------------------
 * @brief     Handler for console command to printout control information
 * @author    S. Wink
 * @date      24. Jan. 2019
 * @param     argc  count of argument list
 * @param     argv  pointer to argument list
 * @return    not equal to zero if error detected
*//*-----------------------------------------------------------------------------------*/
static int cmdVersionHandler_i(int argc, char** argv)
{
    ESP_LOGI(TAG, "version request command received");

    ESP_LOGI(TAG, "Firmware Partnumber: %s", FW_IDENTIFIER);
    ESP_LOGI(TAG, "Firmware Version: %s", FW_VERSION);
    ESP_LOGI(TAG, "Firmware Description: %s", FW_DESCRIPTION);

    return (0);
}


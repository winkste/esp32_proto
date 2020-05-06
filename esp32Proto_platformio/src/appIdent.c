/*****************************************************************************************
* FILENAME :        appIdent.c
*
* DESCRIPTION :
*       This module holds the firmware identification strings and implements
*       an interface to these strings by access functions.
*
* NOTES : N/A
*
* AUTHOR :    Stephan Wink        CREATED ON :    08.03.2019
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

#include "appIdent.h"

#include <stdio.h>

#include "stdint.h"
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
static void RegisterCommands_vd(void);
static int32_t cmdVersionHandler2_s32(int32_t argc_s32, char** argv_cpp, 
                                            FILE *retStream_xp);

/****************************************************************************************/
/* Local variables: */

static const char *TAG              = "appIdent";
static const char *FW_IDENTIFIER    = FWIDENT_STR;  // Firmware identification
static const char *FW_VERSION       = VERSION_STR;  // Firmware Version
static const char *FW_DESCRIPTION   = "prototype firmware for esp32";

/****************************************************************************************/
/* Global functions (unlimited visibility) */

/**---------------------------------------------------------------------------------------
 * @brief     Initialization function for myVersion
*//*-----------------------------------------------------------------------------------*/
esp_err_t appIdent_Initialize_st(void)
{
    RegisterCommands_vd();
    return(ESP_OK);
}

/**---------------------------------------------------------------------------------------
 * @brief     Provides read access to the firmware identifier
*//*-----------------------------------------------------------------------------------*/
const char * appIdent_GetFwIdentifier_cch(void)
{
    return(FW_IDENTIFIER);
}

/**---------------------------------------------------------------------------------------
 * @brief     Provides read access to the firmware version
*//*-----------------------------------------------------------------------------------*/
const char * appIdent_GetFwVersion_cch(void)
{
    return(FW_VERSION);
}

/**---------------------------------------------------------------------------------------
 * @brief     Provides read access to the firmware description
*//*-----------------------------------------------------------------------------------*/
const char * appIdent_GetFwDescription_cch(void)
{
    return(FW_DESCRIPTION);
}

/**---------------------------------------------------------------------------------------
 * @brief     Print the firmware identification to serial
*//*-----------------------------------------------------------------------------------*/
void appIdent_LogFirmwareIdent_vd(const char *tag_cp)
{
    ESP_LOGI(tag_cp, "----------------------------------------------------");
    ESP_LOGI(tag_cp, "Firmware PN: %s", appIdent_GetFwIdentifier_cch());
    ESP_LOGI(tag_cp, "Firmware Version: %s", appIdent_GetFwVersion_cch());
    ESP_LOGI(tag_cp, "Firmware Desc: %s", appIdent_GetFwDescription_cch());
    ESP_LOGI(tag_cp, "----------------------------------------------------");
}

/****************************************************************************************/
/* Local functions: */

/**---------------------------------------------------------------------------------------
 * @brief     Registration of the supported console commands
 * @author    S. Wink
 * @date      24. Jan. 2019
*//*-----------------------------------------------------------------------------------*/
static void RegisterCommands_vd(void)
{
        const myConsole_cmd_t versionCmd = {
        .command = "ver",
        .help = "Get version information",
        .func2 = &cmdVersionHandler2_s32
    };

    ESP_ERROR_CHECK(myConsole_CmdRegister_td(&versionCmd));
}

/**---------------------------------------------------------------------------------------
 * @brief     Handler for console command to printout control information to File stream
 * @author    S. Wink
 * @date      24. Jan. 2019
 * @param     argc_s32  count of argument list
 * @param     argv_cpp  pointer to argument list
 * @param     retStream_xp  stream to print return information to
 * @return    not equal to zero if error detected
*//*-----------------------------------------------------------------------------------*/
static int32_t cmdVersionHandler2_s32(int32_t argc_s32, char** argv_cpp, 
                                            FILE *retStream_xp)
{
    fprintf(retStream_xp, "Firmware Partnumber: %s\n", FW_IDENTIFIER);
    fprintf(retStream_xp, "Firmware Version: %s\n", FW_VERSION);
    fprintf(retStream_xp, "Firmware Description: %s\n", FW_DESCRIPTION);
    fflush(retStream_xp);

    appIdent_LogFirmwareIdent_vd(TAG);
    
    return (0);
}


/*****************************************************************************************
* FILENAME :        mymain.c
*
* DESCRIPTION :
*       This module
*
* AUTHOR :    Stephan Wink        CREATED ON :    24.01.2019
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
#include "esp_log.h"

#include "controlTask.h"

/****************************************************************************************/
/* Local constant defines */

/****************************************************************************************/
/* Local function like makros */

/****************************************************************************************/
/* Local type definitions (enum, struct, union) */

/****************************************************************************************/
/* Local functions prototypes: */

/****************************************************************************************/
/* Local variables: */

static const char *TAG = "mymain";

/****************************************************************************************/
/* Global functions (unlimited visibility) */

/**---------------------------------------------------------------------------------------
 * @brief     Main entry point for application
 * @author    S. Wink
 * @date      24. Jan. 2019
 * @return    n/a
*//*-----------------------------------------------------------------------------------*/
void app_main()
{
    ESP_LOGI(TAG, "starting...");
    controlTask_Initialize_st();
}
/****************************************************************************************/
/* Local functions: */


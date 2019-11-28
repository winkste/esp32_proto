/*****************************************************************************************
* FILENAME :        udpLog.c
*
* DESCRIPTION :
*       This module switches the standard logging to the UDP server logging.
*
* AUTHOR :    Stephan Wink        CREATED ON :    24.03.2019
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

#include "include/udpLog.h"

#include "esp_system.h"
#include "esp_log.h"
#include "esp_err.h"

#include <string.h>

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

/****************************************************************************************/
/* Local constant defines */

#ifndef UDP_LOGGING_MAX_PAYLOAD_LEN
#define UDP_LOGGING_MAX_PAYLOAD_LEN 2048
#endif

/****************************************************************************************/
/* Local function like makros */

/****************************************************************************************/
/* Local type definitions (enum, struct, union) */

typedef enum moduleState_tag
{
    STATE_NOT_INITIALIZED,
    STATE_INITIALIZED,
    STATE_ACTIVATED,
}moduleState_t;

/****************************************************************************************/
/* Local functions prototypes: */
static int32_t GetSocketErrorCode_s32(int32_t socket_s32);
static int32_t ShowSocketErrorReason_s32(int32_t socket_s32);
static int32_t UdpLoggingVPrintf_s32( const char *str_cchp, va_list list_st);

/****************************************************************************************/
/* Local variables: */
static int32_t udpSocket_s32s = 0;
static struct sockaddr_in _sts;
static uint8_t buf_u8s[UDP_LOGGING_MAX_PAYLOAD_LEN];
static vprintf_like_t lastLogFunc_fps;
static const char *TAG = "udpLog";
static moduleState_t moduleState_ens = STATE_NOT_INITIALIZED;



/****************************************************************************************/
/* Global functions (unlimited visibility) */

/**---------------------------------------------------------------------------------------
 * @brief     Function to switch back to original logging
*//*-----------------------------------------------------------------------------------*/
void udpLog_Free_vd(void)
{
	esp_err_t err_st = ESP_OK;

    if(STATE_INITIALIZED != moduleState_ens)
    {
        err_st = ESP_FAIL;
    }
    else
    {
        moduleState_ens = STATE_NOT_INITIALIZED;
        if(NULL != lastLogFunc_fps)
        {
            esp_log_set_vprintf(lastLogFunc_fps);
        }
        err_st = shutdown(udpSocket_s32s, 2);
        if(ESP_OK == err_st)
        {
            ESP_LOGI(TAG, "UDP socket shutdown!");
        }
        else
        {
            ESP_LOGI(TAG, "Shutting-down UDP socket failed: %d!\n", err_st);
        }

        err_st = close(udpSocket_s32s);
        if(ESP_OK == err_st)
        {
            ESP_LOGI(TAG, "UDP socket closed!");
        }else
        {
            ESP_LOGI(TAG, "Closing UDP socket failed: %d!", err_st);
        }
        udpSocket_s32s = 0;
    }
}

/**---------------------------------------------------------------------------------------
 * @brief     Initialization of UDP logging
*//*-----------------------------------------------------------------------------------*/
esp_err_t udpLog_Init_st(const char *ipAddr_cchp, unsigned long port)
{
	struct timeval sendTout_st = {1,0};
	uint32_t ipAddrBytes_s32;
	esp_err_t err_st = ESP_OK;

	if(STATE_NOT_INITIALIZED != moduleState_ens)
	{
	    err_st = ESP_FAIL;
	}
	else
	{
        udpSocket_s32s = 0;
        ESP_LOGI(TAG, "initializing udp logging...");
        udpSocket_s32s = socket(AF_INET, SOCK_DGRAM, 0);

        if(udpSocket_s32s < 0)
        {
           ESP_LOGE(TAG, "Cannot open socket!");
           err_st = ESP_FAIL;
        }
        else
        {
            inet_aton(ipAddr_cchp, &ipAddrBytes_s32);
            ESP_LOGI(TAG, "Logging to 0x%x", ipAddrBytes_s32);

            memset(&_sts, 0, sizeof(_sts));
            _sts.sin_family = AF_INET;
            _sts.sin_port = htons( port );
            _sts.sin_addr.s_addr = ipAddrBytes_s32;

            err_st = setsockopt(udpSocket_s32s, SOL_SOCKET, SO_SNDTIMEO,
                                    (const char *)&sendTout_st, sizeof(sendTout_st));
            if (ESP_OK != err_st)
            {
               ESP_LOGE(TAG, "Failed to set SO_SNDTIMEO. Error %d", err_st);
            }

            lastLogFunc_fps = esp_log_set_vprintf(UdpLoggingVPrintf_s32);
            moduleState_ens = STATE_INITIALIZED;
        }
	}

	return(err_st);
}

/****************************************************************************************/
/* Local functions: */

/**---------------------------------------------------------------------------------------
 * @brief     Returns the socket error code
 * @author    S. Wink
 * @date      24. Mar. 2019
 * @param     socket_s32          UDP socket id
 * @return    n/a
*//*------------------------------------------------------------------------------------*/
static int32_t GetSocketErrorCode_s32(int32_t socket_s32)
{
    int32_t result_s32;
    uint32_t optlen_u32 = sizeof(int);

    result_s32 = getsockopt(socket_s32, SOL_SOCKET, SO_ERROR, &result_s32, &optlen_u32);
    if(-1 == result_s32)
    {
        ESP_LOGE(TAG, "getsockopt failed");
    }

    return(result_s32);
}

/**---------------------------------------------------------------------------------------
 * @brief     prints the socket error reason to the actual logging output
 * @author    S. Wink
 * @date      24. Mar. 2019
 * @param     socket_s32          UDP socket id
 * @return    the actual error code
*//*------------------------------------------------------------------------------------*/
static int32_t ShowSocketErrorReason_s32(int32_t socket_s32)
{
    int32_t err_s32 = GetSocketErrorCode_s32(socket_s32);

    ESP_LOGI(TAG, "UDP socket error %d %s", err_s32, strerror(err_s32));

    return(err_s32);
}

/**---------------------------------------------------------------------------------------
 * @brief     vprintf based logging function to send data to UDP server
 * @author    S. Wink
 * @date      24. Mar. 2019
 * @param     str_cchp          message
 * @param     list_st           variable argument list
 * @return    error code
*//*------------------------------------------------------------------------------------*/
static int32_t UdpLoggingVPrintf_s32( const char *str_cchp, va_list list_st)
{
    int32_t err_s32 = 0;
    int32_t length_s32;
    char taskName_ch[16];
    char *currTask_chp = pcTaskGetTaskName(xTaskGetCurrentTaskHandle());

    strncpy(taskName_ch, currTask_chp, 16);
    taskName_ch[15] = 0;
    if (0 != strncmp(taskName_ch, "tiT", 16))
    {
        length_s32 = vsprintf((char*)buf_u8s, str_cchp, list_st);
        err_s32 = sendto(udpSocket_s32s, buf_u8s, length_s32, 0,
                            (struct sockaddr *)&_sts, sizeof(_sts));
        if(err_s32 < 0)
        {
            ShowSocketErrorReason_s32(udpSocket_s32s);
            vprintf("\nFreeing UDP Logging. sendto failed!\n", list_st);
            udpLog_Free_vd();
            return vprintf("UDP Logging freed!\n\n", list_st);
        }
    }
    return vprintf(str_cchp, list_st);
}



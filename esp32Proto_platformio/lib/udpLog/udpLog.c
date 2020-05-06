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

#include "udpLog.h"

#include "esp_system.h"
#include "esp_log.h"
#include "esp_err.h"

#include <string.h>

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include "utils.h"

// CHANGES TO THIS MODULE
// who shall handle the init and load of the parameter ???
// maybe add one additional module called udppara that takes care on the parameter handling
// parameter:
//  ip address of server
//  port? always standard
//  activated or deactivated? or off/serial/udp/both?
//                                0   0x1    0x2  0x3
//

/****************************************************************************************/
/* Local constant defines */

#ifndef UDP_LOGGING_MAX_PAYLOAD_LEN
    #define UDP_LOGGING_MAX_PAYLOAD_LEN     2048
#endif

#define MODULE_TAG                      "udpLog"

/****************************************************************************************/
/* Local function like makros */

#define CHECK_EXE(arg) utils_CheckAndLogExecution_bol(MODULE_TAG, arg, __LINE__)

/****************************************************************************************/
/* Local type definitions (enum, struct, union) */

typedef enum moduleState_tag
{
    STATE_NOT_INITIALIZED,
    STATE_INITIALIZED,
    STATE_ACTIVATED,
}moduleState_t;

typedef struct moduleData_tag
{
    udpLog_param_t param_st;
    moduleState_t state_en;
    int32_t udpSocket_s32;
    struct sockaddr_in sockAddrIn_st;
    uint8_t buffer_u8[UDP_LOGGING_MAX_PAYLOAD_LEN];
    vprintf_like_t lastLogFunc_fp;
}moduleData_t;

/****************************************************************************************/
/* Local functions prototypes: */
static int32_t GetSocketErrorCode_s32(int32_t socket_s32);
static int32_t ShowSocketErrorReason_s32(int32_t socket_s32);
static int32_t UdpLoggingVPrintf_s32( const char *str_cchp, va_list list_st);

/****************************************************************************************/
/* Local variables: */
static const char *TAG = MODULE_TAG;
static const char *DEFAULT_IP = "192.168.178.89";
static uint32_t DEFAULT_PORT = 1337;

static moduleData_t this_sst =
{
    .state_en = STATE_NOT_INITIALIZED
};

/****************************************************************************************/
/* Global functions (unlimited visibility) */

/**---------------------------------------------------------------------------------------
 * @brief     Initializes the initialization structure of the udplog module
*//*-----------------------------------------------------------------------------------*/
esp_err_t udpLog_InitializeParameter_st(udpLog_param_t *param_stp)
{
    esp_err_t result_st = ESP_FAIL;

    if(NULL != param_stp)
    {
        
        param_stp->ipAddr_cchp = DEFAULT_IP;
        param_stp->conPort_u32 = DEFAULT_PORT;

        

        result_st = ESP_OK;
    }

    return(result_st);
}
/**---------------------------------------------------------------------------------------
 * @brief     Initialization of UDP logging
*//*-----------------------------------------------------------------------------------*/
esp_err_t udpLog_Initialize_st(udpLog_param_t *param_stp)
{
	struct timeval sendTout_st = {1,0};
	uint32_t ipAddrBytes_s32;
	esp_err_t result_st = ESP_OK;
    bool exeResult_bol = true;

	if((STATE_NOT_INITIALIZED != this_sst.state_en) || (NULL == param_stp))
	{
	    exeResult_bol = false;
	}
	else
	{
        memset(&this_sst, 0, sizeof(this_sst));
        memcpy(&this_sst.param_st, param_stp, sizeof(this_sst.param_st));

        ESP_LOGI(TAG, "initializing udp logging...");
        this_sst.udpSocket_s32 = socket(AF_INET, SOCK_DGRAM, 0);

        if(this_sst.udpSocket_s32 < 0)
        {
           ESP_LOGE(TAG, "Cannot open socket!");
           exeResult_bol = false;
        }
        else
        {
            inet_aton(this_sst.param_st.ipAddr_cchp, &ipAddrBytes_s32);
            ESP_LOGI(TAG, "Logging to 0x%x", ipAddrBytes_s32);

            memset(&this_sst.sockAddrIn_st, 0, sizeof(this_sst.sockAddrIn_st));
            this_sst.sockAddrIn_st.sin_family = AF_INET;
            this_sst.sockAddrIn_st.sin_port = htons(this_sst.param_st.conPort_u32);
            this_sst.sockAddrIn_st.sin_addr.s_addr = ipAddrBytes_s32;

            exeResult_bol &= CHECK_EXE(setsockopt(this_sst.udpSocket_s32, 
                                                    SOL_SOCKET, SO_SNDTIMEO,
                                                    (const char *)&sendTout_st, 
                                                    sizeof(sendTout_st)));

            this_sst.lastLogFunc_fp = esp_log_set_vprintf(UdpLoggingVPrintf_s32);
            this_sst.state_en = STATE_INITIALIZED;
        }
	}

    if(true == exeResult_bol)
    {
        result_st = ESP_OK;
        this_sst.state_en = STATE_INITIALIZED;
    }

	return(result_st);
}

/**---------------------------------------------------------------------------------------
 * @brief     Function to switch back to original logging
*//*-----------------------------------------------------------------------------------*/
esp_err_t udpLog_Free_st(void)
{
    esp_err_t result_st = ESP_OK;
    bool exeResult_bol = true;

    if(STATE_INITIALIZED != this_sst.state_en)
    {
        exeResult_bol = false;
    }
    else
    {
        // force reset of module here, independent from later uninit errors
        this_sst.state_en = STATE_NOT_INITIALIZED;

        if(NULL != this_sst.lastLogFunc_fp)
        {
            esp_log_set_vprintf(this_sst.lastLogFunc_fp);
        }
        exeResult_bol &= CHECK_EXE(shutdown(this_sst.udpSocket_s32, 2));

        exeResult_bol &= CHECK_EXE(close(this_sst.udpSocket_s32));
        this_sst.udpSocket_s32 = 0;
    }

    if(true == exeResult_bol)
    {
        result_st = ESP_OK;
    }

	return(result_st);
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
        length_s32 = vsprintf((char*)this_sst.buffer_u8, str_cchp, list_st);
        err_s32 = sendto(this_sst.udpSocket_s32, this_sst.buffer_u8, length_s32, 0,
                            (struct sockaddr *)&this_sst.sockAddrIn_st, 
                            sizeof(this_sst.sockAddrIn_st));
        if(err_s32 < 0)
        {
            ShowSocketErrorReason_s32(this_sst.udpSocket_s32);
            vprintf("\nFreeing UDP Logging. sendto failed!\n", list_st);
            udpLog_Free_st();
            return vprintf("UDP Logging freed!\n\n", list_st);
        }
    }
    return vprintf(str_cchp, list_st);
}



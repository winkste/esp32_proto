/****************************************************************************************
* FILENAME :        consoleSocket.c
*
* DESCRIPTION :
*       This module handles the console tcp socket connection
*
* AUTHOR :    Stephan Wink        CREATED ON :    24.01.2019
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
****************************************************************************************/

/***************************************************************************************/
/* Include Interfaces */

#include "consoleSocket.h"

#include <stdio.h>
#include "stdlib.h"
#include "string.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "lwip/err.h"
#include "lwip/errno.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include "esp_err.h"


#include "sdkconfig.h"
#include "myConsole.h"

/***************************************************************************************/
/* Local constant defines */
#ifndef BIT0
    #define BIT0    0x00000000
    #define BIT1    0x00000001
    #define BIT2    0x00000002
#endif

#define PORT CONFIG_EXAMPLE_PORT

#define RX_BUFFER_SIZE  5000U

/***************************************************************************************/
/* Local function like makros */

/***************************************************************************************/
/* Local type definitions (enum, struct, union) */

/***************************************************************************************/
/* Local functions prototypes: */
static void ExecuteTcpSocket(void);
static esp_err_t ExecuteCommand(char *cmdBuffer_cp, FILE *outStream_xp);
static esp_err_t StartConnection(char* addr_cp, int* listenSocket_ip,
                                 char** rxBuffer_chpp);
static void StopConnection(int sock_i, int listenSocket_i, char* rxBuffer_chp);
static void StopSocket(int sock_i);

/***************************************************************************************/
/* Local variables: */
static const char *TAG = "consoleSocket";
static const int START_SOCKET_SERVER = BIT0;
static EventGroupHandle_t socketServerEventGroup_sts;
void (*eventSocketError_ptrs)(void);
static TaskHandle_t *const socketServer_ptrs;

/***************************************************************************************/
/* Global functions (unlimited visibility) */
/**--------------------------------------------------------------------------------------
 * @brief     Initialization function for socket server. This function has to
 *                  be called first.
 * @author    S. Wink
 * @date      24. Jan. 2019
 * @param     param_stp             parameter set for initialization
 * @return    ESP based error code (ESP_OK)
*//*-----------------------------------------------------------------------------------*/
esp_err_t consoleSocket_Initialize_st(socketServer_parameter_t *param_stp)
{
    esp_err_t success_st = ESP_OK;

    ESP_LOGI(TAG, "initializing...");
    eventSocketError_ptrs = param_stp->eventSocketError_ptrs;
    socketServerEventGroup_sts = xEventGroupCreate();
    xTaskCreate(consoleSocket_Task_vd, "socketServer", 4096, NULL, 5, socketServer_ptrs);
    ESP_LOGI(TAG, "task created...");

    return(success_st);
}

/**--------------------------------------------------------------------------------------
 * @brief     Activates the socket server
 * @author    S. Wink
 * @date      24. Jan. 2019
*//*-----------------------------------------------------------------------------------*/
void consoleSocket_Activate_vd(void)
{
    ESP_LOGI(TAG, "set START_SOCKET_SERVER event...");
    xEventGroupSetBits(socketServerEventGroup_sts, START_SOCKET_SERVER);
}

/**--------------------------------------------------------------------------------------
 * @brief     Deactivates the socket server
 * @author    S. Wink
 * @date      24. Jan. 2019
*//*-----------------------------------------------------------------------------------*/
void consoleSocket_Deactivate_vd(void)
{
    ESP_LOGI(TAG, "delete socket server...");
    vTaskDelete(socketServer_ptrs);
}

/**--------------------------------------------------------------------------------------
 * @brief     Task function of socket server
 * @author    S. Wink
 * @date      24. Jan. 2019
 * @param     pvParameters      parameter set, interface defined from freertos
*//*-----------------------------------------------------------------------------------*/
void consoleSocket_Task_vd(void *pvParameters)
{
    uint32_t bits = START_SOCKET_SERVER;
    EventBits_t uxBits_st;

    while(1)
    {
        ESP_LOGI(TAG, "socket server waiting for activation...");
        uxBits_st = xEventGroupWaitBits(socketServerEventGroup_sts, bits,
                                        true, false, portMAX_DELAY);

        if(0 != (uxBits_st & START_SOCKET_SERVER))
        {
            ESP_LOGI(TAG, "START_SOCKET_SERVER received...");
            ExecuteTcpSocket();

        }
    }
}

/***************************************************************************************/
/* Local functions: */
/**--------------------------------------------------------------------------------------
 * @brief     executes the server connection
 * @author    S. Wink
 * @date      24. Jan. 2019
*//*-----------------------------------------------------------------------------------*/
static void ExecuteTcpSocket(void)
{
    int32_t listenSock_s32;
    int32_t workSock_s32;
    char *rxBuffer_chp;
    char addr_str[128];
    int32_t result_s32;
    struct sockaddr_in6 sourceAddr_st; // Large enough for both IPv4 or IPv6
    socklen_t addrLen_st = sizeof(sourceAddr_st);
    int32_t length_s32;


    if(ESP_FAIL == StartConnection(addr_str, &listenSock_s32, &rxBuffer_chp))
    {
        return;
    }

    while (1)
    {
        ESP_LOGI(TAG, "Start accepting connections");
        workSock_s32 = accept(listenSock_s32, (struct sockaddr *)&sourceAddr_st,
                                &addrLen_st);
        if (workSock_s32 < 0) {
            ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno); // @suppress("Symbol is not resolved")
            break;
        }
        ESP_LOGI(TAG, "Socket accepted");

        while (1)
        {
            length_s32 = recv(workSock_s32, rxBuffer_chp,
                                sizeof(char) * RX_BUFFER_SIZE - 1, 0);

            if (length_s32 < 0) // Error occured during receiving
            {
                ESP_LOGE(TAG, "recv failed: errno %d", errno); // @suppress("Symbol is not resolved")
                StopSocket(workSock_s32);
                break;
            }
            else if (length_s32 == 0) // Connection closed
            {
                ESP_LOGI(TAG, "Connection closed");
                StopSocket(workSock_s32);
                break;
            }
            // Data received
            else
            {
                // Get the sender's ip address as string
                if (sourceAddr_st.sin6_family == PF_INET)
                {
                    inet_ntoa_r(((struct sockaddr_in *)&sourceAddr_st)->sin_addr.s_addr,
                                    addr_str, sizeof(addr_str) - 1);
                } else if (sourceAddr_st.sin6_family == PF_INET6) {
                    inet6_ntoa_r(sourceAddr_st.sin6_addr, addr_str,
                                    sizeof(addr_str) - 1);
                }

                // Null-terminate whatever we received and treat like a string
                *(rxBuffer_chp + length_s32) = '\0';
                *(rxBuffer_chp + length_s32 + 1) = '\n';
                ESP_LOGI(TAG, "Received %d bytes from %s", length_s32, addr_str);

                FILE *outStream_xp;
                char *buf;
                size_t len;
                outStream_xp = open_memstream(&buf, &len);

                if(ESP_OK == ExecuteCommand(rxBuffer_chp, outStream_xp))
                {
                    if(0 != len)
                    {
                        sprintf(rxBuffer_chp, "%s", buf);
                    }
                    else
                    {
                        sprintf(rxBuffer_chp, "OK\r\n");
                    }               
                }
                else
                {
                    ESP_LOGI(TAG, "Unrecognized msg: %s", rxBuffer_chp);
                    sprintf(rxBuffer_chp, "ERROR\r\n");
                }

                fclose(outStream_xp);
                free(buf);


                length_s32 = strlen(rxBuffer_chp);
                result_s32 = send(workSock_s32, rxBuffer_chp, length_s32, 0U);
                if (result_s32 < 0)
                {
                    ESP_LOGE(TAG, "Error occured during sending: err =  %d", result_s32);
                    StopSocket(workSock_s32);
                    break;
                }
            }
        }
    }

    StopConnection(workSock_s32, listenSock_s32, rxBuffer_chp);
    ESP_LOGE(TAG, "stop socket execution...");
}

/**--------------------------------------------------------------------------------------
 * @brief     executes the received command
 * @author    S. Wink
 * @date      24. Jan. 2019
 * @param     cmdBuffer_cp  Buffer to command data which shall be executed
 * @return    returns ESP_OK if success, in all other cases ESP_FAIL
*//*-----------------------------------------------------------------------------------*/
static esp_err_t ExecuteCommand(char *cmdBuffer_cp, FILE *outStream_xp)
{
    esp_err_t cmdExeResult_st = ESP_FAIL;
    esp_err_t err_st = ESP_FAIL;
    int32_t cmdResponse_s32 = 0;

    err_st = myConsole_Run2_td(cmdBuffer_cp, &cmdResponse_s32, outStream_xp);

    if (ESP_ERR_NOT_FOUND == err_st)
    {
        ESP_LOGW(TAG, "Unrecognized command");
    }
    else if (ESP_ERR_INVALID_ARG == err_st)
    {
        ESP_LOGW(TAG, "command was empty");
    }
    else if ((ESP_OK == err_st) && (ESP_OK != cmdResponse_s32))
    {
        ESP_LOGE(TAG, "Command returned non-zero error code: 0x%x (%s)",
                cmdResponse_s32, esp_err_to_name(err_st));
    }
    else if (ESP_OK != err_st)
    {
        ESP_LOGE(TAG, "Internal error: %s", esp_err_to_name(err_st));
    }
    else if((ESP_OK != err_st) || (ESP_OK != cmdResponse_s32))
    {
       ESP_LOGE(TAG,
            "Unexpected error combination occured during command execution");
       ESP_LOGE(TAG, "error combination: err = %d, resp = %d",
            err_st, cmdResponse_s32);
    }
    else
    {
        ESP_LOGI(TAG, "command successful executed...");
        cmdExeResult_st = ESP_OK;
    }

    return(cmdExeResult_st);
}

/**--------------------------------------------------------------------------------------
 * @brief     starts a socket connection
 * @author    S. Wink
 * @date      24. Jan. 2019
 * @param     cmdBuffer_cp    Buffer to command data which shall be executed
 * @param     listenSocket_ip pointer to the generated listener socket
 * @param     rxBuffer_chpp   pointer to pointer of the allocated receive buffer
 * @return    returns ESP_OK if success, in all other cases ESP_FAIL
*//*-----------------------------------------------------------------------------------*/
static esp_err_t StartConnection(char* addr_cp, int* listenSocket_ip,
                                    char** rxBuffer_chpp)
{
    int err;
    int addr_family;
    int ip_protocol;
    int y = 1;
#ifdef CONFIG_EXAMPLE_IPV4
    struct sockaddr_in destAddr;
    destAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons(PORT);
    addr_family = AF_INET;
    ip_protocol = IPPROTO_IP;
    inet_ntoa_r(destAddr.sin_addr, addr_cp, sizeof(addr_cp) - 1);
#else // IPV6
    struct sockaddr_in6 destAddr;
    bzero(&destAddr.sin6_addr.un, sizeof(destAddr.sin6_addr.un));
    destAddr.sin6_family = AF_INET6;
    destAddr.sin6_port = htons(PORT);
    addr_family = AF_INET6;
    ip_protocol = IPPROTO_IPV6;
    inet6_ntoa_r(destAddr.sin6_addr, addr_str, sizeof(addr_str) - 1);
#endif

    ESP_LOGI(TAG, "Starting socket server...");
    *listenSocket_ip = socket(addr_family, SOCK_STREAM, ip_protocol);
    if (*listenSocket_ip < 0)
    {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno); // @suppress("Symbol is not resolved")
        return(ESP_FAIL);
    }
    ESP_LOGI(TAG, "Socket created");

    err = setsockopt(*listenSocket_ip, SOL_SOCKET, SO_REUSEADDR, &y, sizeof(int));
    if (err != 0) {
        ESP_LOGE(TAG, "Socket unable set option: errno %d", errno); // @suppress("Symbol is not resolved")
        return(ESP_FAIL);
    }

    err = bind(*listenSocket_ip, (struct sockaddr *)&destAddr, sizeof(destAddr));
    if (err != 0) {
        ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno); // @suppress("Symbol is not resolved")
        return(ESP_FAIL);
    }
    ESP_LOGI(TAG, "Socket binded");

    err = listen(*listenSocket_ip, 1);
    if (err != 0) {
        ESP_LOGE(TAG, "Error occured during listen: errno %d", errno); // @suppress("Symbol is not resolved")
        return(ESP_FAIL);
    }
    ESP_LOGI(TAG, "Socket listening");

    ESP_LOGI(TAG, "Allocate memory for rx buffer...");
    *rxBuffer_chpp = malloc(sizeof(char) * RX_BUFFER_SIZE);
    if(NULL == *rxBuffer_chpp)
    {
        ESP_LOGI(TAG, "Unable to allocate memory for rx buffer...");
        return(ESP_FAIL);
    }
    memset(*rxBuffer_chpp, 0U, sizeof(char) * RX_BUFFER_SIZE);

    return(ESP_OK);
}

/**--------------------------------------------------------------------------------------
 * @brief     stops the socket connection
 * @author    S. Wink
 * @date      24. Jan. 2019
 * @param     cmdBuffer_cp    Buffer to command data which shall be executed
 * @param     listenSocket_ip pointer to the generated listener socket
 * @param     rxBuffer_chp    pointer to pointer of the allocated receive buffer
*//*-----------------------------------------------------------------------------------*/
static void StopConnection(int sock_i, int listenSocket_i, char* rxBuffer_chp)
{
    ESP_LOGI(TAG, "stop socket connection...");

    StopSocket(sock_i);
    StopSocket(listenSocket_i);

    free(rxBuffer_chp);

    xEventGroupClearBits(socketServerEventGroup_sts, START_SOCKET_SERVER);
    return;
}

/**--------------------------------------------------------------------------------------
 * @brief     stops the socket connection
 * @author    S. Wink
 * @date      24. Jan. 2019
 * @param     sock_i        Socket to shutdown
*//*-----------------------------------------------------------------------------------*/
static void StopSocket(int sock_i)
{
    if (sock_i != -1)
    {
        ESP_LOGE(TAG, "Shutting down socket: %d", sock_i);
        shutdown(sock_i, 0);
        close(sock_i);
    }
}



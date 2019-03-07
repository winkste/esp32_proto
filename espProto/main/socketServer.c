/*****************************************************************************************
* FILENAME :        socketServer.c
*
* DESCRIPTION :
*       This module handles the tcp socket connection
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
#include "socketServer.h"

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

#include "myConsole.h"

/****************************************************************************************/
/* Local constant defines */
#ifndef BIT0
    #define BIT0    0x00000000
    #define BIT1    0x00000001
    #define BIT2    0x00000002
#endif

#define PORT CONFIG_EXAMPLE_PORT

/****************************************************************************************/
/* Local function like makros */

/****************************************************************************************/
/* Local type definitions (enum, struct, union) */

/****************************************************************************************/
/* Local functions prototypes: */
static void executeTcpSocket(void);

/****************************************************************************************/
/* Local variables: */
static const int START_SOCKET_SERVER   = BIT0;
static EventGroupHandle_t socketServerEventGroup_sts;
static const char *TAG = "socketServer";
void (*eventSocketError_ptrs)(void);
static TaskHandle_t * const socketServer_ptrs;

/****************************************************************************************/
/* Global functions (unlimited visibility) */
/**---------------------------------------------------------------------------------------
 * @brief     Initialization function for socket server. This function has to
 *                  be called first.
 * @author    S. Wink
 * @date      24. Jan. 2019
 * @param     param_stp             parameter set for initialization
 * @return    ESP based error code (ESP_OK)
*//*-----------------------------------------------------------------------------------*/
esp_err_t socketServer_Initialize_st(socketServer_parameter_t *param_stp)
{
    esp_err_t success_st = ESP_OK;

    ESP_LOGI(TAG, "initializing...");
    eventSocketError_ptrs = param_stp->eventSocketError_ptrs;
    socketServerEventGroup_sts = xEventGroupCreate();
    xTaskCreate(socketServer_Task_vd, "socketServer", 4096, NULL, 5, socketServer_ptrs);
    ESP_LOGI(TAG, "task created...");

    return(success_st);
}

/**---------------------------------------------------------------------------------------
 * @brief     Activates the socket server
 * @author    S. Wink
 * @date      24. Jan. 2019
*//*-----------------------------------------------------------------------------------*/
void socketServer_Activate_vd(void)
{
    ESP_LOGI(TAG, "set START_SOCKET_SERVER event...");
    xEventGroupSetBits(socketServerEventGroup_sts, START_SOCKET_SERVER);
}

/**---------------------------------------------------------------------------------------
 * @brief     Deactivates the socket server
 * @author    S. Wink
 * @date      24. Jan. 2019
*//*-----------------------------------------------------------------------------------*/
void socketServer_Deactivate_vd(void)
{
    ESP_LOGI(TAG, "delete socket server...");
    vTaskDelete(socketServer_ptrs);
}

/**---------------------------------------------------------------------------------------
 * @brief     Task function of socket server
 * @author    S. Wink
 * @date      24. Jan. 2019
 * @param     pvParameters      parameter set, interface defined from freertos
*//*-----------------------------------------------------------------------------------*/
void socketServer_Task_vd(void *pvParameters)
{
    uint32_t bits = START_SOCKET_SERVER;
    EventBits_t uxBits_st;

    while(1)
    {
        ESP_LOGI(TAG, "socket server waiting for activation...");
        uxBits_st = xEventGroupWaitBits(socketServerEventGroup_sts, bits,
                                        true, false, portMAX_DELAY); // @suppress("Symbol is not resolved")

        if(0 != (uxBits_st & START_SOCKET_SERVER))
        {
            ESP_LOGI(TAG, "START_SOCKET_SERVER received...");
            executeTcpSocket();

        }
    }
}


/****************************************************************************************/
/* Local functions: */
/**---------------------------------------------------------------------------------------
 * @brief     executes the server connection
 * @author    S. Wink
 * @date      24. Jan. 2019
*//*-----------------------------------------------------------------------------------*/
static void executeTcpSocket(void)
{
    char rx_buffer[128];
    char addr_str[128];
    int addr_family;
    int ip_protocol;
    int y = 1;
    //int8_t cmdResp_s8 = 0U;
#ifdef CONFIG_EXAMPLE_IPV4
    struct sockaddr_in destAddr;
    destAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons(PORT);
    addr_family = AF_INET;
    ip_protocol = IPPROTO_IP;
    inet_ntoa_r(destAddr.sin_addr, addr_str, sizeof(addr_str) - 1);
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
    int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
    if (listen_sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno); // @suppress("Symbol is not resolved")
        return;
    }
    ESP_LOGI(TAG, "Socket created");

    int err = setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &y, sizeof(int));
    if (err != 0) {
        ESP_LOGE(TAG, "Socket unable set option: errno %d", errno); // @suppress("Symbol is not resolved")
        return;
    }

    err = bind(listen_sock, (struct sockaddr *)&destAddr, sizeof(destAddr));
    if (err != 0) {
        ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno); // @suppress("Symbol is not resolved")
        return;
    }
    ESP_LOGI(TAG, "Socket binded");

    err = listen(listen_sock, 1);
    if (err != 0) {
        ESP_LOGE(TAG, "Error occured during listen: errno %d", errno); // @suppress("Symbol is not resolved")
        return;
    }
    ESP_LOGI(TAG, "Socket listening");

    while (1)
    {
        ESP_LOGI(TAG, "Start accepting connections");
        struct sockaddr_in6 sourceAddr; // Large enough for both IPv4 or IPv6
        uint addrLen = sizeof(sourceAddr);
        int sock = accept(listen_sock, (struct sockaddr *)&sourceAddr, &addrLen);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno); // @suppress("Symbol is not resolved")
            break;
        }
        ESP_LOGI(TAG, "Socket accepted");

        while (1) {
            int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
            // Error occured during receiving
            if (len < 0)
            {
                ESP_LOGE(TAG, "recv failed: errno %d", errno); // @suppress("Symbol is not resolved")
                break;
            }
            // Connection closed
            else if (len == 0)
            {
                ESP_LOGI(TAG, "Connection closed");
                break;
            }
            // Data received
            else
            {
                // Get the sender's ip address as string
                if (sourceAddr.sin6_family == PF_INET) {
                    inet_ntoa_r(((struct sockaddr_in *)&sourceAddr)->sin_addr.s_addr, addr_str, sizeof(addr_str) - 1);
                } else if (sourceAddr.sin6_family == PF_INET6) {
                    inet6_ntoa_r(sourceAddr.sin6_addr, addr_str, sizeof(addr_str) - 1);
                }

                //memcpy(stationWifiSettings_sts.sta.ssid, rx_buffer, sizeof(stationWifiSettings_sts.sta.ssid));
                rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
                ESP_LOGI(TAG, "Received %d bytes from %s:", len, addr_str);
                ESP_LOGI(TAG, "%s", rx_buffer);

                int ret;
                err = myConsole_Run_td(rx_buffer, &ret);
                if (err == ESP_ERR_NOT_FOUND) {
                    ESP_LOGW(TAG, "Unrecognized command\n");
                } else if (err == ESP_ERR_INVALID_ARG) {
                    // command was empty
                } else if (err == ESP_OK && ret != ESP_OK) {
                    ESP_LOGE(TAG, "Command returned non-zero error code: 0x%x (%s)\n", ret, esp_err_to_name(err));
                } else if (err != ESP_OK) {
                    ESP_LOGE(TAG, "Internal error: %s\n", esp_err_to_name(err));
                }

                //commands_Execute(rx_buffer, len);

                /*cmdResp_s8 = CheckForCommand_s8(rx_buffer, len);
                if(1 == cmdResp_s8)
                {
                    ESP_LOGE(TAG, "Shutting down socket...");
                    shutdown(sock, 0);
                    close(sock);
                    shutdown(listen_sock, 0);
                    close(listen_sock);
                    ESP_LOGE(TAG, "Disconnect wifi...");
                    ESP_LOGE(TAG, "Restarting wifi setup in station mode...");
                    xEventGroupClearBits(socketServerEventGroup_sts, START_SOCKET_SERVER);
                    InitializeWifiSta();
                    return;
                }
                else if(2 == cmdResp_s8)
                {
                    ESP_LOGE(TAG, "Shutting down socket...");
                    shutdown(sock, 0);
                    close(sock);
                    shutdown(listen_sock, 0);
                    close(listen_sock);
                    ESP_LOGE(TAG, "Disconnect wifi...");
                    ESP_LOGE(TAG, "Restarting wifi setup in station mode...");
                    xEventGroupClearBits(socketServerEventGroup_sts, START_SOCKET_SERVER);
                    InitializeWifiSoftAp();
                    return;
                }*/

                int err = send(sock, rx_buffer, len, 0);
                if (err < 0)
                {
                    ESP_LOGE(TAG, "Error occured during sending: errno %d", errno); // @suppress("Symbol is not resolved")
                    break;
                }
            }
        }

        if (sock != -1)
        {
            ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
            shutdown(listen_sock, 0);
            close(listen_sock);
            xEventGroupClearBits( socketServerEventGroup_sts, START_SOCKET_SERVER);
            return;
        }
    }
    ESP_LOGE(TAG, "stop socket execution...");
}

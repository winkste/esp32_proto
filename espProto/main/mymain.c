/* BSD Socket API Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_event_legacy.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/errno.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include "tcpip_adapter.h"

#include "controlTask.h"

//#include "basic.h"

/******************************************************************************/
/* The examples use simple WiFi configuration that you can set via
   'make menuconfig'.
   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/

#define EXAMPLE_ESP_WIFI_SSID      CONFIG_WIFI_SSID//"test"
#define EXAMPLE_ESP_WIFI_PASS      CONFIG_WIFI_PASSWORD
#define EXAMPLE_ESP_MAXIMUM_RETRY  CONFIG_ESP_MAXIMUM_RETRY
#define CONFIG_MAX_CONN_RETRIES       2
#define EXAMPLE_ESP_AP_WIFI_SSID      "myssid3_"
#define EXAMPLE_ESP_AP_WIFI_PASS      "testtest"
#define EXAMPLE_MAX_AP_STA_CONN       2

//#define EXAMPLE_WIFI_SSID CONFIG_WIFI_SSID
//#define EXAMPLE_WIFI_PASS CONFIG_WIFI_PASSWORD
#define PORT CONFIG_EXAMPLE_PORT

#ifndef BIT0
    #define BIT0    0x00000000
    #define BIT1    0x00000001
#define BIT1        0x00000002
#endif

int socketActive_bol = 0U;
/******************************************************************************/
/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t wifi_event_group;

const int IPV4_GOTIP_BIT = BIT0;
const int IPV6_GOTIP_BIT = BIT1;

static EventGroupHandle_t socketServerEventGroup_sts;
static const int START_SOCKET_SERVER = BIT0;

TaskHandle_t socketTask_sts = NULL;
uint8_t retryConnectCounter_u8sta = 0U;

static const char *TAG = "wifi_mgr";

wifi_config_t stationWifiSettings_sts =
{
    .sta =
    {
        .ssid = EXAMPLE_ESP_WIFI_SSID,
        .password = EXAMPLE_ESP_WIFI_PASS
    },
};

wifi_config_t apWifiSettings_sts =
{
    .ap =
    {
        .ssid = EXAMPLE_ESP_AP_WIFI_SSID,
        .ssid_len = strlen(EXAMPLE_ESP_AP_WIFI_SSID),
        .password = EXAMPLE_ESP_AP_WIFI_PASS,
        .max_connection = EXAMPLE_MAX_AP_STA_CONN,
        .authmode = WIFI_AUTH_WPA_WPA2_PSK
    },
};

/******************************************************************************/
static void InitializeWifi();
static void InitializeWifiSta();
static void InitializeWifiSoftAp();
//static void StopWifi();
//static void tcp_server_task(void *pvParameters);
static esp_err_t event_handler(void *ctx, system_event_t *event);
static int8_t CheckForCommand_s8(char *rxBuffer_cp, int length_u16);

/******************************************************************************/
static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    ESP_LOGI(TAG, "event handler called");

    switch(event->event_id)
    {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        ESP_LOGI(TAG, "SYSTEM_EVENT_STA_START");
        break;
    case SYSTEM_EVENT_STA_CONNECTED:
        ESP_LOGI(TAG, "SYSTEM_EVENT_STA_CONNECTED");
        // enable ipv6
        tcpip_adapter_create_ip6_linklocal(TCPIP_ADAPTER_IF_STA);
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        ESP_LOGI(TAG, "SYSTEM_EVENT_STA_GOT_IP");
        //xTaskCreate(tcp_server_task, "tcp_server", 4096, NULL, 5, socketTask_sts);
        //ESP_LOGI(TAG, "server task started...");
        break;
    case SYSTEM_EVENT_AP_STA_GOT_IP6:
        ESP_LOGI(TAG, "SYSTEM_EVENT_STA_GOT_IP6");
        char *ip6 = ip6addr_ntoa(&event->event_info.got_ip6.ip6_info.ip);
        ESP_LOGI(TAG, "IPv6: %s", ip6);
        ESP_LOGI(TAG, "Task Value: %u", (uint32_t)socketTask_sts);
        /*SWIif(NULL == socketTask_sts)
        {
            xTaskCreate(tcp_server_task, "tcp_server", 4096, NULL, 5, &socketTask_sts);
            ESP_LOGE(TAG, "server task started...");
            //ESP_LOGI(TAG, "Task Value: %u", (uint32_t)socketTask_sts);
        }*/
        xEventGroupSetBits(socketServerEventGroup_sts, START_SOCKET_SERVER);
        ESP_LOGI(TAG, "START_SOCKET_SERVER event fired...");
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        ESP_LOGI(TAG, "SYSTEM_EVENT_STA_DISCONNECTED");
        /*swiif(NULL != socketTask_sts)
        {
            ESP_LOGI(TAG, "delete existing server task...");
            vTaskDelete(socketTask_sts);
        }*/
        if(CONFIG_MAX_CONN_RETRIES > retryConnectCounter_u8sta)
        {
            /* This is a workaround as ESP32 WiFi libs don't currently
            auto-reassociate.*/
            esp_wifi_connect();
            ESP_LOGI(TAG,"retry to connect to the AP");
            retryConnectCounter_u8sta++;
        }
        else
        {
            retryConnectCounter_u8sta = 0U;
            ESP_LOGI(TAG,"connection to AP failed, start own AP");
            InitializeWifiSoftAp();
        }
        break;

    case SYSTEM_EVENT_AP_STACONNECTED:
        ESP_LOGI(TAG, "SYSTEM_EVENT_AP_STACONNECTED");
        ESP_LOGI(TAG, "station2:"MACSTR" join, AID=%d",
                 MAC2STR(event->event_info.sta_connected.mac),
                 event->event_info.sta_connected.aid);
        //SWIxTaskCreate(tcp_server_task, "tcp_server", 4096, NULL, 5, &socketTask_sts);
        //ESP_LOGI(TAG, "server task started...");
        xEventGroupSetBits(socketServerEventGroup_sts, START_SOCKET_SERVER);
        ESP_LOGI(TAG, "START_SOCKET_SERVER event fired...");
        break;
    case SYSTEM_EVENT_AP_STADISCONNECTED:
        ESP_LOGI(TAG, "SYSTEM_EVENT_AP_STADISCONNECTED");
        ESP_LOGI(TAG, "station:"MACSTR"leave, AID=%d",
                 MAC2STR(event->event_info.sta_disconnected.mac),
                 event->event_info.sta_disconnected.aid);
        /*swiif(NULL != socketTask_sts)
        {
            vTaskDelete(socketTask_sts);
        }*/
        break;
    default:
        break;
    }
    return ESP_OK;
}

static void InitializeWifi()
{
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
}

static void InitializeWifiSoftAp()
{
    /*wifi_config_t wifi_config =
    {
        .ap =
        {
            .ssid = EXAMPLE_ESP_AP_WIFI_SSID,
            .ssid_len = strlen(EXAMPLE_ESP_AP_WIFI_SSID),
            .password = EXAMPLE_ESP_AP_WIFI_PASS,
            .max_connection = EXAMPLE_MAX_AP_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };*/

    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_ERROR_CHECK(esp_wifi_deinit());
    //ESP_ERROR_CHECK(esp_wifi_disconnect());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    //ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &apWifiSettings_sts));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished.SSID:%s password:%s",
             EXAMPLE_ESP_AP_WIFI_SSID, EXAMPLE_ESP_AP_WIFI_PASS);
}

static void InitializeWifiSta()
{
    /*wifi_config_t wifi_config =
    {
        .sta =
        {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS
        },
    };*/

    //ESP_ERROR_CHECK(esp_wifi_disconnect());
    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &stationWifiSettings_sts));
    //ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");
    ESP_LOGI(TAG, "connect to ap SSID:%s password:%s",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
}

/*static void StopWifi(void)
{
    ESP_ERROR_CHECK(esp_wifi_disconnect());
    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_ERROR_CHECK(esp_wifi_deinit());
}*/

/*static void initialise_wifi(void)
{
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    wifi_config_t wifi_config = {
        .ap = {
            .ssid="_ESP32_b",
            .ssid_len=0,
            .password="12345678",
            .channel=6,
            .authmode=WIFI_AUTH_WPA2_PSK,
            .ssid_hidden=0,
            .max_connection=4,
            .beacon_interval=100
        }
    };

    tcpip_adapter_init();

    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config) );
    ESP_ERROR_CHECK( esp_wifi_start() );
}*/

/*
static void wait_for_ip()
{
    uint32_t bits = IPV4_GOTIP_BIT | IPV6_GOTIP_BIT ;

    ESP_LOGI(TAG, "Waiting for AP connection...");
    xEventGroupClearBits(wifi_event_group, IPV4_GOTIP_BIT);
    xEventGroupClearBits(wifi_event_group, IPV6_GOTIP_BIT);
    xEventGroupWaitBits(wifi_event_group, bits, false, false, portMAX_DELAY); // @suppress("Symbol is not resolved")
    ESP_LOGI(TAG, "Connected to AP");
}*/

static int8_t CheckForCommand_s8(char *rxBuffer_cp, int length_u16)
{
    int8_t returnValue_s8 = 0;

    if(length_u16 > 5)
    {
        if(0 > strcmp("$sid ", rxBuffer_cp))
        {
            ESP_LOGI(TAG, "SSID command, parameter: %s", &rxBuffer_cp[5]);
            memset(stationWifiSettings_sts.sta.ssid, 0, sizeof(stationWifiSettings_sts.sta.ssid));
            memcpy(stationWifiSettings_sts.sta.ssid, &rxBuffer_cp[5], (length_u16 - 5));
            ESP_LOGI(TAG, "set SSID value: %s", stationWifiSettings_sts.sta.ssid);
        }
        else if(0 > strcmp("$pwd ", rxBuffer_cp))
        {
            ESP_LOGI(TAG, "password command, parameter: %s", &rxBuffer_cp[5]);
            memset(stationWifiSettings_sts.sta.password, 0, sizeof(stationWifiSettings_sts.sta.password));
            memcpy(stationWifiSettings_sts.sta.password, &rxBuffer_cp[5], (length_u16 - 5));
            ESP_LOGI(TAG, "set SSID value: %s", stationWifiSettings_sts.sta.password);
        }
        else if(0 > strcmp("$cons ", rxBuffer_cp))
        {
            returnValue_s8 = 1;
        }
        else if(0 > strcmp("$cona ", rxBuffer_cp))
        {
            returnValue_s8 = 2;
        }
        else
        {
            ESP_LOGI(TAG, "unknown command and parameter structure: %s", rxBuffer_cp);
            returnValue_s8 = -1;
        }
    }

    return(returnValue_s8);
}

static void executeTcpSocket(void)
{
    char rx_buffer[128];
    char addr_str[128];
    int addr_family;
    int ip_protocol;
    int y = 1;
    int8_t cmdResp_s8 = 0U;
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

                cmdResp_s8 = CheckForCommand_s8(rx_buffer, len);
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
                }

                int err = send(sock, rx_buffer, len, 0);
                if (err < 0) {
                    ESP_LOGE(TAG, "Error occured during sending: errno %d", errno); // @suppress("Symbol is not resolved")
                    break;
                }
            }
        }

        if (sock != -1) {
            ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
    }
    ESP_LOGE(TAG, "stop socket execution...");
}

static void tcpSocketTask(void *pvParameters)
{
    uint32_t bits = START_SOCKET_SERVER;
    EventBits_t uxBits_st;

    socketServerEventGroup_sts = xEventGroupCreate();
    while(1)
    {
        ESP_LOGI(TAG, "socket server waiting...");
        uxBits_st = xEventGroupWaitBits(socketServerEventGroup_sts, bits,
                                        true, false, portMAX_DELAY); // @suppress("Symbol is not resolved")

        if(0 != (uxBits_st & START_SOCKET_SERVER))
        {
            ESP_LOGI(TAG, "Server start flag received...");
            executeTcpSocket();

        }
    }
}
/*
static void tcp_server_task(void *pvParameters)
{
    char rx_buffer[128];
    char addr_str[128];
    int addr_family;
    int ip_protocol;
    int y = 1;

    while (1) {

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
            break;
        }
        ESP_LOGI(TAG, "Socket created");

        int err = setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &y, sizeof(int));
        if (err != 0) {
            ESP_LOGE(TAG, "Socket unable set option: errno %d", errno); // @suppress("Symbol is not resolved")
            break;
        }

        err = bind(listen_sock, (struct sockaddr *)&destAddr, sizeof(destAddr));
        if (err != 0) {
            ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno); // @suppress("Symbol is not resolved")
            break;
        }
        ESP_LOGI(TAG, "Socket binded");

        err = listen(listen_sock, 1);
        if (err != 0) {
            ESP_LOGE(TAG, "Error occured during listen: errno %d", errno); // @suppress("Symbol is not resolved")
            break;
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

                    if(1U == CheckForCommand(rx_buffer, len))
                    {
                        ESP_LOGE(TAG, "Shutting down socket...");
                        shutdown(sock, 0);
                        close(sock);
                        ESP_LOGE(TAG, "Disconnect wifi...");
                        //StopWifi();
                        //vTaskDelete(socketTask_sts);
                        //vTaskDelay(500 / portTICK_PERIOD_MS);
                        ESP_LOGE(TAG, "Restarting wifi setup in station mode...");
                        //InitializeWifiSta();
                        break;
                    }

                    int err = send(sock, rx_buffer, len, 0);
                    if (err < 0) {
                        ESP_LOGE(TAG, "Error occured during sending: errno %d", errno); // @suppress("Symbol is not resolved")
                        break;
                    }
                }
            }

            if (sock != -1) {
                ESP_LOGE(TAG, "Shutting down socket and restarting...");
                shutdown(sock, 0);
                close(sock);
            }
        }
    }
    ESP_LOGE(TAG, "delete task from task...");
    vTaskDelete(NULL);
}*/

void app_main()
{
    //basic_Activate_bol();
    ESP_LOGI(TAG, "starting flash...");
    //ESP_ERROR_CHECK( nvs_flash_init() );
    //xTaskCreate(tcpSocketTask, "tcpSocketTask", 4096, NULL, 5, NULL);
    ESP_LOGI(TAG, "starting wifi...");
    controlTask_Initialize_st();
/*
    InitializeWifi();
    InitializeWifiSta();
    //initialise_wifi();
    //SWIwait_for_ip();

    xTaskCreate(tcpSocketTask, "tcpSocketTask", 4096, NULL, 5, NULL);
*/
}

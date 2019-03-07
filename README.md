# esp32_proto
esp32 prototype development within the idf freertos environment

### Python scripts
Script tcpclient.py contains configuration for port number, IP version (IPv4 or IPv6) and IP address that has to be altered to match the values used by the application. Example:

```
PORT = 3333;
IP_VERSION = 'IPv4'
IPV4 = '192.168.4.1'
IPV6 = 'FE80::32AE:A4FF:FE80:5288'
```

## Hardware Required

This application can be run on any commonly available ESP32 development board.

## Configure the project

```
make menuconfig
```

Set following parameter under Serial Flasher Options:

* Set `Default serial port`.

Set following parameters under Example Configuration Options:

* Set `WiFi SSID` of the Router (Access-Point).

* Set `WiFi Password` of the Router (Access-Point).

* Set `IP version` of the example to be IPV4 or IPV6.

* Set `Port` number of the socket, that server example will create.

## Build and Flash

Build the project and flash it to the board, then run monitor tool to view serial output:

```
make -j4 flash monitor
```

(To exit the serial monitor, type ``Ctrl-]``.)

See the Getting Started Guide for full steps to configure and use ESP-IDF to build projects.


## Troubleshooting

Start server first, to receive data sent from the client (application).

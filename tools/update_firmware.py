#!/usr/bin/env python

#
#  update_firmware.py
#  esp32-ota
#
#  Script to upload firmware for OTA demo application
#
#  Created by Andreas Schweizer on 08.12.2016.
#  Copyright (C) 2016 Classy Code GmbH
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import sys
import os
import binascii
import socket
import time


class UpdateFirmware(object):

    def __init__(self, targetIpAddress, binFilePath):
        self.targetIpAddress = targetIpAddress
        self.binFilePath = binFilePath

    def run(self):
        self.connect()
        
        result1 = self.send_start_command()
        if result1 != "OK\r\n":
            raise RuntimeError("failed: %s" % result1)

        result2 = self.send_file()
        if result2 != "OK\r\n":
            raise RuntimeError("failed: %s" % result2)
        
        self.send_end_command()
        self.send_reboot_command()
        self.disconnect()

    # Open TCP socket to target device.
    def connect(self):
        print("connecting to %s..." % self.targetIpAddress)
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.connect((self.targetIpAddress, 3333))
        print("connected, socket = %s" % self.socket)
    
    # Close TCP socket.
    def disconnect(self):
        self.socket.close()
        self.socket = None
    
    # Send file over TCP socket in binary chunks of max 4 kBytes.
    def send_file(self):
        chunkSize = 512#4096
        sizeBytes = os.path.getsize(self.binFilePath)
        totalNofChunks = (sizeBytes + chunkSize - 1) / chunkSize
        with open(self.binFilePath, "rb") as f:
            chunkNr = 1
            while True:
                chunk = f.read(chunkSize)
                if chunk:
                    print("sending fragment %d of %d (%d bytes)" % (chunkNr, totalNofChunks, len(chunk)))
                    #print(chunk)
                    #print(chunk.decode('ASCII'))
                    #print("%02x %02x %02x %02x" %chunk[0], chunk[1], chunk[2], chunk[3])
                    #print(chunk.encode("hex"))
                    chunk2 = chunk.encode("hex")
                    #h2 = ("ota w -l %d" %len(chunk2)) + " -d " + chunk2
                    h3 = ("ota w -l %d -s%d" %(len(chunk2), chunkNr)) + " -d " + chunk2
                    #print(h2) 
                    #h = ("ota w -l %04x" % (len(chunk) + 5)) + " -d " + chunk
                    #h = ("ota w -l %d" % len(chunk)) + " -d " + " \""+ chunk + " \""
                    result1 = self.send_command(h3)
                    if result1 != "OK\r\n":
                        return result1
                    chunkNr = chunkNr + 1
                else:
                    break;
        return "OK\r\n"

    # Send OTA start command to target.
    def send_start_command(self):
        print("sending 'start OTA' command to target...")
        return self.send_command("ota b\n")
    
    # Send OTA complete command to target.
    def send_end_command(self):
        print("sending 'end OTA' command to target...")
        return self.send_command("ota e\n")

    # Re-boot target.
    def send_reboot_command(self):
        print("sending 'reboot' command to target...")
        return self.send_command("boot \n")

    def send_command(self, cmd):
        totalSent = 0
        while totalSent < len(cmd):
            #myString = cmd[totalSent:]
            #myByte = myString.encode()
            sent = self.socket.send(cmd[totalSent:])
            #sent = self.socket.send(myByte)
            if sent == 0:
                raise RuntimeError("Socket connection broken")
            totalSent = totalSent + sent
            
        # print("Sent %d bytes, waiting for response..." % totalSent)

        chunks = []
        bytesReceived = 0
        while 1:
            chunkByte = self.socket.recv(1)
            chunk = chunkByte.decode()
            if chunk == b'':
                raise RuntimeError("Socket connection broken")
            chunks.append(chunk)
            bytesReceived = bytesReceived + len(chunk)
            if chunk[-1:] == '\n':
                break
        data = b''.join(chunks)
        # print("receive complete, data = '%s'" % (data.replace("\n", "").replace("\r", "")))
        return data

if __name__ == '__main__':
    if len(sys.argv) < 2:
        sys.exit("Usage: %s <ip-address> <input.bin>" % sys.argv[0])

    updater = UpdateFirmware(sys.argv[1], sys.argv[2])
    updater.run()
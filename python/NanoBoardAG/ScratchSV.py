#!/usr/local/bin/python
# -*- coding: utf-8 -*-
################################################################################
# ScratchSV.py
# Description:  Scratch Remote Sensor Connections Server
#
# Author:       shozo fukuda
# Date:         Sun Sep 07 19:37:36 2014
# Last revised: $Date$
# Application:  Python 2.7
################################################################################

#<IMPORT>
import os,sys
import socket
import struct
import numbers
import threading
import re

#<CONFIG>#######################################################################
# Description:  
# Dependencies: 
################################################################################
HOST = 'localhost'
PORT = 42001

sym = r'\s*"(.*)"'
num = r'\s*((+|-)?[1-9]\d*(\.\d+)?)'

#<CLASS>########################################################################
# Description:  
# Dependencies: 
################################################################################
class ScratchSV(dict):
    def __init__(self, host=HOST, name="aaa"):
        self.name = name
        # connect to Scratch
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.connect((host, PORT))

        # run receiver for Scratch
        self.receiver = threading.Thread(target=self.recv)
        self.receiver.daemon = True
        self.receiver.start()

    def send(self, cmd):
        msg = struct.pack('!L', len(cmd)) + cmd
        self.socket.sendall(msg)

    def broadcast(self, s):
        self.send('broadcast "%s"' %(s))

    def sensor_update(self, name, val):
        if isinstance(val, bool):
            s = str(val).lower()
        elif isinstance(val, numbers.Real):
            s = str(val)
        elif isinstance(val, str):
            s = '"'+val+'"'
        self.send('sensor-update "%s" %s' %(name, s))

    def _read(self, n):
        while len(self.buff) < n:
            self.buff += self.socket.recv(1024)
        (s, self.buff) = (self.buff[:n], self.buff[n:])
        return s

    def recv_broadcast(self, m_arg):
        print 'broadcast:'+m_arg

    def recv_sensor_update(self, m_arg):
        print 'sensor-update:'+m_arg
        
    def recv(self):
        self.buff = ''
        while True:
            size = struct.unpack('!L', self._read(4))[0]
            
            (m_type, _, m_arg) = self._read(size).partition(' ')
            if m_type == 'broadcast':
                self.recv_broadcast(m_arg)
            elif m_type == 'sensor-update':
                self.recv_sensor_update(m_arg)

#<SUBROUTINE>###################################################################
# Function:     
# Description:  
# Dependencies: 
################################################################################
#unquoted single-word strings (cat, mouse-x)
#quoted strings ("a four word string", "embedded ""quotation marks"" are doubled")
#numbers (1, -1, 3.14, -1.2, .1, -.2)
#booleans (true or false)

#<TEST>#########################################################################
# Function:     テストルーチン
# Description:  
# Dependencies: 
################################################################################
if __name__ == '__main__':
  print "TEST: ScratchSV.py"
  s = ScratchSV()

# ScratchSV.py


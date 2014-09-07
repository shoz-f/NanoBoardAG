#!/usr/local/bin/python
# -*- coding: utf-8 -*-
################################################################################
# NanoBoardAG.py
# Description:  なのぼ～どAG用ドライバ
#
# Author:       shozo fukuda
# Date:         Mon Aug 11 08:31:02 2014
# Application:  Python 2.7.6
#
#<参考文献>#####################################################################
#
# 1)Scratch Board Technical Information
#   http://www.etecaprigio.com.br/scratch/ScratchBoard_Tech_InfoR2.pdf
# 2)なのぼ～どAG最新ファームウエア
#   http://tiisai.dip.jp/wordpress/wp-content/uploads/NanoBoardAGWithMotorTrio_diag_sonerm412.zip
# 3)なのぼ～どAG用Scratch - NanoBoardAGWithMotor.cs
#   http://squeakland.jp/abee/tmp/NanoBoardAGWithMotor.zip
#
################################################################################

#<IMPORT>
import serial
import struct

class NanoBoardAG:
    """ なのぼ～どAG ドライバ """

    def __init__(self, com=4):
        """ NanoBoardAGが接続されているシリアルポートを開き、初期化する
            デフォルトのポートはCOM5
        """
        self.port = serial.Serial(com, 38400, timeout=1)

        # 内部状態を初期化
        self.id     = 0    # ch15
        self.sensor = [0,0,0,0,0,0,0,0]

    def send_cmd(self):
        """ NanoBoardにコマンドを送る """
        self.port.write('\x01')
 
    def update(self):
        """ NanoBoardにコマンドを送り、状態をアップデートする """
        # コマンド送信
        self.port.flushInput()
        self.send_cmd()
        res = self.port.read(18)

        # センサ入力をデコードする
        for h in struct.unpack('>9H', res):
            ch  = ((h & 0x7800) >> 11)
            val = ((h & 0x0700) >>  1) | (h & 0x007F)
            if ch == 15:
                self.id = val
            else:
                self.sensor[ch] = val

    # センサー読み取り
    @property
    def resistanceD(self):
        """ Ch 0: アナログ・チャネルD """
        self.update()
        return round((100*self.sensor[0])/1023.0)

    @property
    def resistanceC(self):
        """ Ch 1: アナログ・チャネルC """
        self.update()
        return round((100*self.sensor[1])/1023.0)

    @property
    def resistanceB(self):
        """ Ch 2: アナログ・チャネルB """
        self.update()
        return round((100*self.sensor[2])/1023.0)

    @property
    def button(self):
        """ Ch 3: ボタン・スイッチ """
        self.update()
        return (self.sensor[3] != 0)

    @property
    def resistanceA(self):
        """ Ch 4: アナログ・チャネルA """
        self.update()
        return ((100*self.sensor[4])/1023.0)

    @property
    def light(self):
        """ Ch 5: 光センサ """
        self.update()
        if self.sensor[5] < 25:
            return 100 - self.sensor[5]
        else:
            return round((1023 - self.sensor[5])*(75/998.0))

    @property
    def sound(self):
        """ Ch 6: 音センサ― """
        self.update()
        val = max(0, self.sensor[6] - 18)
        if val < 50:
            return val/2
        else:
            return 25 + min(75, round((val - 50)*(75/580.0)))

    @property
    def slider(self):
        """ Ch7: スライダー """
        self.update()
        return round((100*self.sensor[7])/1023.0)


class NanoBoardAGMotor1(NanoBoardAG):
    """ なのぼ～どAG w/モーターひとつ用のドライバ """

    def __init__(self, com=4):
        """ NanoBoardAGが接続されているシリアルポートを開く """
        NanoBoardAG.__init__(self, com)

        self.run       = False
        self.speed     = 0
        self.direction = 0

    def send_cmd(self):
        """ NanoBoardにコマンドを送る """
        if self.run:
            self.port.write(chr((self.direction << 7) | int(0x7f * self.speed/100.0)))
        else:
            self.port.write('\x00')

    # モーター制御
    def motorOn(self):
        """ モーター回転ON """
        self.run = True
        self.update()

    def motorOff(self):
        """ モーター回転OFF """
        self.run = False
        self.update()

    def motorDirection(self, direction):
        """ モーターの回転方向を設定する
              0:正回転, 1:逆回転, 2:反転
        """
        if direction == 0:
            self.direction = 0
        elif direction == 1:
            self.direction = 1
        elif direction == 2:
            self.direction ^= 1
        self.update()

    def motorSpeed(self, speed):
        """ モーターのスピードを設定する """
        if speed < 0:
            self.speed = 0
        elif speed > 100:
            self.speed = 100
        else:
            self.speed = speed
        self.update()


if __name__ == '__main__':
    nano = NanoBoardAGMotor1()
    print nano.slider
    nano.motorSpeed(50)
    nano.motorOn()


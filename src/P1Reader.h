/*
 * Copyright (c) 2023 Jean-Pierre sneyers
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Additionally, please note that the original source code of this file
 * may contain portions of code derived from (or inspired by)
 * previous works by:
 *
 * Ronald Leenes (https://github.com/romix123/P1-wifi-gateway and http://esp8266thingies.nl)
 */

#ifndef P1READER_H
#define P1READER_H

#define DISABLED 0
#define WAITING 1
#define READING 2
#define CHECKSUM 3
#define DONE 4
#define FAILURE 5
#define FAULT 6

#define MAXLINELENGTH 2048 // 0-0:96.13.0 has a maximum lenght of 1024 chars + 11 of its identifier

#include <Arduino.h>
#include <TimeLib.h>
#include "GlobalVar.h"
#include "Debug.h"

class P1Reader
{
private:
  settings &conf;
  unsigned long nextUpdateTime = millis() + 5000; //wait 5s before read datagram
  bool OEstate = false; // 74125 OE output enable is off by default (EO signal high)
  void RTS_on();
  void RTS_off();
  void alignToTelegram();
  int dataFailureCount = 0;
  void OBISparser(int len);
  String readFirstParenthesisVal(int start, int end);
  String readBetweenDoubleParenthesis(int start, int end);
  int FindCharInArray(char array[], char c, int len);
  void decodeTelegram(int len);
  unsigned int CRC16(unsigned int crc, unsigned char *buf, int len);
  String identifyMeter(String Name);
  String readUntilStar(int start, int end);

public:
  int state = DISABLED;
  unsigned long LastSample = 0;
  explicit P1Reader(settings &currentConf);
  unsigned long GetnextUpdateTime();
  void DoMe();
  void readTelegram();
  void ResetnextUpdateTime();
  char telegram[MAXLINELENGTH] = {}; // holds a single line of the datagram
  String datagram;                   // holds entire datagram for raw output
  String meterName = "";
  bool datagramValid = false;
  bool dataEnd = false; // signals that we have found the end char in the data (!)
  struct DataP1
  {
    char gasReceived5min[12];
    char gasDomoticz[12]; // Domoticz wil gas niet in decimalen?
    String P1header;
    char P1version[8];
    int P1prot; // 4 or 5 based on P1version 1-3:0.2.8
    char P1timestamp[30] = "\0";
    char equipmentId[100] = "\0";
    char equipmentId2[100] = "\0";
    char electricityUsedTariff1[12];
    char electricityUsedTariff2[12];
    char electricityReturnedTariff1[12];
    char electricityReturnedTariff2[12];
    char tariffIndicatorElectricity[8];
    char numberPowerFailuresAny[6];
    char numberLongPowerFailuresAny[6];
    String longPowerFailuresLog;
    char numberVoltageSagsL1[7];
    char numberVoltageSagsL2[7];
    char numberVoltageSagsL3[7];
    char numberVoltageSwellsL1[7];
    char numberVoltageSwellsL2[7];
    char numberVoltageSwellsL3[7];
    String textMessage;
    char instantaneousVoltageL1[7];
    char instantaneousVoltageL2[7];
    char instantaneousVoltageL3[7];
    char instantaneousCurrentL1[9];
    char instantaneousCurrentL2[9];
    char instantaneousCurrentL3[9];
    char activePowerL1P[10];
    char activePowerL2P[10];
    char activePowerL3P[10];
    char activePowerL1NP[10];
    char activePowerL2NP[10];
    char activePowerL3NP[10];
    char actualElectricityPowerDeli[14];
    char actualElectricityPowerRet[14];
  } DataReaded = {};
};
#endif
/*
 * Copyright (c) 2025 Jean-Pierre Sneyers
 * Source : https://github.com/narfight/P1-wifi-gateway
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

#include <Arduino.h>
#include "GlobalVar.h"
#include "Debug.h"

#define MAXLINELENGTH 1037 // 0-0:96.13.0 has a maximum lenght of 1024 chars + 11 of its identifier + end line (2char)
#define P1TIMEOUTREAD 10000

enum class State {
  DISABLED,
  WAITING,
  READING,
  DONE,
  FAULT
};

class P1Reader
{
public:
  State state = State::DISABLED;
  unsigned long LastSample = 0;
  explicit P1Reader(settings &currentConf);
  unsigned long GetnextUpdateTime();
  char telegram[MAXLINELENGTH] = {}; // holds a single line of the datagram
  String datagram;                   // holds entire datagram for raw output
  String meterName = "";
  bool dataEnd = false; // signals that we have found the end char in the data (!)
  void DoMe();
  void readTelegram();
  void ResetnextUpdateTime();

  /// @brief value that is parsed as a three-decimal float, but stored as an
  // integer (by multiplying by 1000). Supports val() (or implicit cast to
  // float) to get the original value, and int_val() to get the more
  // efficient integer value. The unit() and int_unit() methods on
  // FixedField return the corresponding units for these values.
  struct FixedValue
  {
    FixedValue() = default;
    explicit FixedValue(String value) : _value(value.toFloat() * 1000) {}

    operator float() const { return _value * 0.001f; }
    float val() const { return _value * 0.001f; }

  private:
    uint32_t _value = 0;
  };

  struct DataP1
  {
    char gasReceived5min[12];
    char gasDomoticz[12]; // Domoticz wil gas niet in decimalen?
    char P1version[8];
    char P1timestamp[13] = "\0";
    char equipmentId[100] = "\0";
    char equipmentId2[100] = "\0";
    FixedValue electricityUsedTariff1;
    FixedValue electricityUsedTariff2;
    FixedValue electricityReturnedTariff1;
    FixedValue electricityReturnedTariff2;
    uint32_t tariffIndicatorElectricity;
    uint32_t numberPowerFailuresAny;
    uint32_t numberLongPowerFailuresAny;
    String longPowerFailuresLog;
    uint32_t numberVoltageSagsL1;
    uint32_t numberVoltageSagsL2;
    uint32_t numberVoltageSagsL3;
    uint32_t numberVoltageSwellsL1;
    uint32_t numberVoltageSwellsL2;
    uint32_t numberVoltageSwellsL3;
    String textMessage;
    FixedValue instantaneousVoltageL1;
    FixedValue instantaneousVoltageL2;
    FixedValue instantaneousVoltageL3;
    FixedValue instantaneousCurrentL1;
    FixedValue instantaneousCurrentL2;
    FixedValue instantaneousCurrentL3;
    FixedValue activePowerL1P;
    FixedValue activePowerL2P;
    FixedValue activePowerL3P;
    FixedValue activePowerL1NP;
    FixedValue activePowerL2NP;
    FixedValue activePowerL3NP;
    FixedValue actualElectricityPowerDeli;
    FixedValue actualElectricityPowerRet;
  } DataReaded = {};
  void OnNewDatagram(std::function<void()> callback)
  {
    delegates.push_back(callback);
  }

protected:
  void TriggerCallbacks()
  {
    for(const auto& callback : delegates) 
    {
      if(callback) callback();
    }
  }
private:
  std::vector<std::function<void()>> delegates;
  settings &conf;
  unsigned long nextUpdateTime = millis() + 5000; //wait 5s before read datagram
  unsigned long TimeOutRead;
  void RTS_on();
  void RTS_off();
  void OBISparser(int len);
  String readFirstParenthesisVal(int start, int end);
  String readBetweenDoubleParenthesis(int start, int end);
  int FindCharInArray(const char array[], char c, int len);
  void decodeTelegram(int len);
  String identifyMeter(String Name);
  String readUntilStar(int start, int end);
  bool CheckTimeout();
};
#endif
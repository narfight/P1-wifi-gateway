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

#include "P1Reader.h"

P1Reader::P1Reader(settings &currentConf) : conf(currentConf)
{
  Serial.setRxBufferSize(MAXLINELENGTH-2);
  Serial.begin(SERIALSPEED);
  datagram.reserve(1500);
}

void P1Reader::RTS_on() // switch on Data Request
{
  MainSendDebug("[P1] Data requested");
  Serial.flush(); //flush output buffer
  while(Serial.available() > 0 ) Serial.read(); //flush input buffer
  
  state = State::WAITING; // signal that we are waiting for a valid start char (aka /)
  digitalWrite(OE, LOW); // enable buffer
  digitalWrite(DR, HIGH); // turn on Data Request
}

void P1Reader::RTS_off() // switch off Data Request
{
  state = State::DISABLED;
  nextUpdateTime = millis() + conf.interval * 1000;
  digitalWrite(DR, LOW); // turn off Data Request
  digitalWrite(OE, HIGH); // put buffer in Tristate mode
}

void P1Reader::ResetnextUpdateTime()
{
  RTS_off();
  nextUpdateTime = 0;
}

int P1Reader::FindCharInArray(const char array[], char c, int len)
{
  for (int i = 0; i < len; i++)
  {
    if (array[i] == c)
    {
      return i;
    }
  }
  return -1;
}

String P1Reader::identifyMeter(String Name)
{
  if (Name.indexOf("FLU5\\") != -1)
  {
    return "Siconia"; //Belgium
  }
  if (Name.indexOf("ISK5\\2M550E-1011") != -1)
  {
    return "ISKRA AM550e-1011";
  }
  if (Name.indexOf("KFM5KAIFA-METER") != -1)
  {
    return "Kaifa  (MA105 of MA304)";
  }
  if (Name.indexOf("XMX5LGBBFG10") != -1)
  {
    return "Landis + Gyr E350";
  }
  if (Name.indexOf("XMX5LG") != -1)
  {
    return "Landis + Gyr";
  }
  if (Name.indexOf("Ene5\\T210-D") != -1)
  {
    return "Sagemcom T210-D";
  }
  return "UNKNOW";
}

void P1Reader::decodeTelegram(int len)
{
  int startChar = FindCharInArray(telegram, '/', len);
  int endChar = FindCharInArray(telegram, '!', len);

  if (state == State::WAITING) // we're waiting for a valid start sequence, if this line is not it, just return
  {
    if (startChar >= 0)
    { // start found. Reset CRC calculation
      MainSendDebug("[P1] Start of datagram found");
      
      digitalWrite(DR, LOW); // turn off Data Request
      digitalWrite(OE, HIGH); // put buffer in Tristate mode
      
      // reset datagram
      datagram = "";
      dataEnd = false;
      state = State::READING;

      for (int cnt = startChar; cnt < len - startChar; cnt++)
      {
        datagram += telegram[cnt];
      }

      if (meterName == "")
      {
        meterName = identifyMeter(telegram);
      }

      return;
    }
    else
    {
      return; // We're waiting for a valid start char, if we're in the middle of a datagram, just return.
    }
  }

  if (state == State::READING)
  {
    if (endChar >= 0)
    { // we have found the endchar !
      MainSendDebug("[P1] End found");
      dataEnd = true; // we're at the end of the data stream, so mark (for raw data output) We don't know if the data is valid, we will test this below.
     
      if (datagram.length() < 2048)
      {
        for (int cnt = 0; cnt < len; cnt++)
        {
          datagram += telegram[cnt];
        }
        datagram += "\r\n";
      }
      else
      {
        MainSendDebug("[P1] Buffer overflow ?");
        state = State::FAULT;
        return;
      }

      state = State::DONE;
      LastSample = millis();
      return;
    }
    else
    { // no endchar, so normal line, process
      for (int cnt = 0; cnt < len; cnt++)
      {
        datagram += telegram[cnt];
      }
      OBISparser(len);
    }
    return;
  }
  return;
}

String P1Reader::readFirstParenthesisVal(int start, int end)
{
  String value = "";
  int i = start + 1;
  bool trailingZero = true;
  while ((telegram[i] != ')') && (i < end))
  {
    if (trailingZero && telegram[i] == '0')
    {
      i++;
    }
    else
    {
      value += (char)telegram[i];
      trailingZero = false;
      i++;
    }
  }
  return value;
}

String P1Reader::readUntilStar(int start, int end)
{
  String value = "";
  int i = start + 1;
  bool trailingZero = true;
  while ((telegram[i] != '*') && (i < end))
  {
    if (trailingZero && telegram[i] != '0')
    {
      trailingZero = false;
    }
    if (telegram[i] == '0' && telegram[i + 1] == '.')
    {
      // value += (char)telegram[i];
      trailingZero = false;
    }
    if (!trailingZero)
    {
      value += (char)telegram[i];
    }
    i++;
  }
  return value;
}

String P1Reader::readBetweenDoubleParenthesis(int start, int end)
{
  String value = "";
  int i = start + 1;
  while ((telegram[i] != ')') && (telegram[i + 1] != '('))
  {
    i++; // we have found the intersection of the command and data
         // 0-1:24.2.1(231029141500W)(05446.465*m3)
  }
  i++;
  value = readUntilStar(i, end);
  return value;
}

void P1Reader::OBISparser(int len)
{
  int i;
  String value;
  String inString = "";
  int idx;

  for (i = 0; i < len; i++)
  {
    if (telegram[i] == '(')
    {
      break;
    }
    if (isDigit(telegram[i]))
    {
      // convert the incoming byte to a char and add it to the string:
      inString += (char)telegram[i];
    }
  }

  idx = inString.toInt();
  
  switch (idx)
  {
  case 0:
    break;
  case 9614: // device type
    readFirstParenthesisVal(i, len).toCharArray(DataReaded.P1version, sizeof(DataReaded.P1version));
    break;
  case 100:
    readFirstParenthesisVal(i, len).toCharArray(DataReaded.P1timestamp, sizeof(DataReaded.P1timestamp));
    break;
  case 96140:
    DataReaded.tariffIndicatorElectricity = readFirstParenthesisVal(i, len).toInt();
    if (conf.InverseHigh_1_2_Tarif)
    {
      if (DataReaded.tariffIndicatorElectricity == 1)
      {
        DataReaded.tariffIndicatorElectricity = 2;
      }
      else
      {
        DataReaded.tariffIndicatorElectricity = 1;
      }
    }
    break;
  case 9611:
    readFirstParenthesisVal(i, len).toCharArray(DataReaded.equipmentId, sizeof(DataReaded.equipmentId));
    break;
  case 19610:
    readFirstParenthesisVal(i, len).toCharArray(DataReaded.equipmentId2, sizeof(DataReaded.equipmentId2));
    break;
  case 9810:
    //Maximum demand – Active energy import of the last 13 months
    break;
  case 10170: // 1-0:1.7.0 – actualElectricityPowerDelivered
    DataReaded.actualElectricityPowerDeli = FixedValue(readUntilStar(i, len));
    break;
  case 10270: // 1-0:1.7.0 – actualElectricityPowerReturned
    DataReaded.actualElectricityPowerRet = FixedValue(readUntilStar(i, len));
    break;
  case 10181: // 1-0:1.8.1(000992.992*kWh) Elektra verbruik laag tarief
    if (!conf.InverseHigh_1_2_Tarif)
    {
      DataReaded.electricityUsedTariff1 = FixedValue(readUntilStar(i, len));
    }
    else
    {
      DataReaded.electricityUsedTariff2 = FixedValue(readUntilStar(i, len));
    }
    break;
  case 10182: // 1-0:1.8.2(000560.157*kWh) = Elektra verbruik hoog tarief
    if (!conf.InverseHigh_1_2_Tarif)
    {
      DataReaded.electricityUsedTariff2 = FixedValue(readUntilStar(i, len));
    }
    else
    {
      DataReaded.electricityUsedTariff1 = FixedValue(readUntilStar(i, len));
    }
    break;
  case 10281: // 1-0:2.8.1(000348.890*kWh) Elektra opbrengst laag tarief
    if (!conf.InverseHigh_1_2_Tarif)
    {
      DataReaded.electricityReturnedTariff1 = FixedValue(readUntilStar(i, len));
    }
    else
    {
      DataReaded.electricityReturnedTariff2 = FixedValue(readUntilStar(i, len));
    }
    break;
  case 10282: // 1-0:2.8.2(000859.885*kWh) Elektra opbrengst hoog tarief
    if (!conf.InverseHigh_1_2_Tarif)
    {
      DataReaded.electricityReturnedTariff2 = FixedValue(readUntilStar(i, len));
    }
    else
    {
      DataReaded.electricityReturnedTariff1 = FixedValue(readUntilStar(i, len));
    }
    break;
  case 103170: // 1-0:31.7.0(002*A) Instantane stroom Elektriciteit L1
    DataReaded.instantaneousCurrentL1 = FixedValue(readUntilStar(i, len));
    break;
  case 105170: // 1-0:51.7.0(002*A) Instantane stroom Elektriciteit L2
    DataReaded.instantaneousCurrentL2 = FixedValue(readUntilStar(i, len));
    break;
  case 107170: // 1-0:71.7.0(002*A) Instantane stroom Elektriciteit L3
    DataReaded.instantaneousCurrentL3 = FixedValue(readUntilStar(i, len));
    break;
  case 103270: // 1-0:32.7.0(232.0*V) Voltage L1
    DataReaded.instantaneousVoltageL1 = FixedValue(readUntilStar(i, len));
   break;
  case 105270: // 1-0:52.7.0(232.0*V) Voltage L2
    DataReaded.instantaneousVoltageL2 = FixedValue(readUntilStar(i, len));
    break;
  case 107270: // 1-0:72.7.0(232.0*V) Voltage L3
    DataReaded.instantaneousVoltageL3 = FixedValue(readUntilStar(i, len));
    break;
  case 102170: // 1-0:21.7.0(002*A) Instantane stroom Elektriciteit L1
    DataReaded.activePowerL1P = FixedValue(readUntilStar(i, len));
   break;
  case 104170: // 1-0:41.7.0(002*A) Instantane stroom Elektriciteit L2
    DataReaded.activePowerL2P = FixedValue(readUntilStar(i, len));
    break;
  case 106170: // 1-0:61.7.0(002*A) Instantane stroom Elektriciteit L3
    DataReaded.activePowerL3P = FixedValue(readUntilStar(i, len));
    break;
  case 102270: // 1-0:22.7.0(232.0*V) Voltage L1
    DataReaded.activePowerL1NP = FixedValue(readUntilStar(i, len));
   break;
  case 104270: // 1-0:42.7.0(232.0*V) Voltage L2
    DataReaded.activePowerL2NP = FixedValue(readUntilStar(i, len));
    break;
  case 106270: // 1-0:62.7.0(232.0*V) Voltage L3
    DataReaded.activePowerL3NP = FixedValue(readUntilStar(i, len));
    break;
  case 1032320: // Aantal korte spanningsdalingen Elektriciteit in fase 1
    DataReaded.numberVoltageSagsL1 = readFirstParenthesisVal(i, len).toInt();
    break;
  case 1052320: // Aantal korte spanningsdalingen Elektriciteit in fase 2
   DataReaded.numberVoltageSagsL2 = readFirstParenthesisVal(i, len).toInt();
    break;
  case 1072320: // Aantal korte spanningsdalingen Elektriciteit in fase 3
    DataReaded.numberVoltageSagsL3 = readFirstParenthesisVal(i, len).toInt();
    break;
  case 1032360: // 1-0:32.36.0(00000) Aantal korte spanningsstijgingen Elektriciteit in fase 1
    DataReaded.numberVoltageSwellsL1 = readFirstParenthesisVal(i, len).toInt();
    break;
  case 1052360: // 1-0:52.36.0(00000) Aantal korte spanningsstijgingen Elektriciteit in fase 2
    DataReaded.numberVoltageSwellsL2 = readFirstParenthesisVal(i, len).toInt();
    break;
  case 1072360: // 1-0:72.36.0(00000) Aantal korte spanningsstijgingen Elektriciteit in fase 3
    DataReaded.numberVoltageSwellsL3 = readFirstParenthesisVal(i, len).toInt();
    break;
  case 12421: // gas
    readBetweenDoubleParenthesis(i, len).toCharArray(DataReaded.gasReceived5min, sizeof(DataReaded.gasReceived5min));
    readBetweenDoubleParenthesis(i, len).toCharArray(DataReaded.gasDomoticz, sizeof(DataReaded.gasDomoticz));
    break;
  case 96721: // 0-0:96.7.21(00051)  Number of power failures in any phase
    DataReaded.numberPowerFailuresAny = readFirstParenthesisVal(i, len).toInt();
    break;
  case 9679: // 0-0:96.7.9(00007) Number of long power failures in any phase
    DataReaded.numberLongPowerFailuresAny = readFirstParenthesisVal(i, len).toInt();
    break;
  case 1099970: // 1-0:99.97.0(6) Power Failure Event Log (long power failures)
    DataReaded.longPowerFailuresLog = "";
    while (i < len)
    {
      DataReaded.longPowerFailuresLog += (char)telegram[i];
      i++;
    }
    break;
    case 10140:
    case 10160:
    case 96310:
    case 1700:
    case 103140:
    case 96130:
    //ignore line :-)
    break;
  default:
    MainSendDebugPrintf("[P1] Unrecognized line : %s", inString);
    break;
  }
  // clear the string for new input:
  inString = "";
}

unsigned long P1Reader::GetnextUpdateTime()
{
  return nextUpdateTime;
}

void P1Reader::DoMe()
{
  if ((millis() > nextUpdateTime) && state == State::DISABLED)
  {
    RTS_on();
  }

  if (state == State::WAITING || state == State::READING)
  {
    readTelegram();
  }
}

void P1Reader::readTelegram()
{
  if (Serial.available())
  {
    unsigned long TimeOutRead = millis() + 5000; //max read time : 5s

    memset(telegram, 0, sizeof(telegram));
    while (Serial.available())
    {
      if (millis() > TimeOutRead)
      {
        MainSendDebug("[P1] Timeout");
        RTS_off();
        return;
      }

      int len = Serial.readBytesUntil('\n', telegram, sizeof(telegram)-2);
      telegram[len] = '\n';
      telegram[len + 1] = 0;
      
      decodeTelegram(len + 1);

      if (state == State::DONE)
      {
        blink(1, 400);
        RTS_off();
        TriggerCallbacks();
      }
    }
  }
}
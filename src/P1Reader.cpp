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
  
  state = WAITING; // signal that we are waiting for a valid start char (aka /)
  digitalWrite(OE, LOW); // enable buffer
  digitalWrite(DR, HIGH); // turn on Data Request
  OEstate = true;
}

void P1Reader::RTS_off() // switch off Data Request
{
  MainSendDebugPrintf("[P1] Data end request. Next action in %dms", nextUpdateTime);
  
  digitalWrite(DR, LOW); // turn off Data Request
  digitalWrite(OE, HIGH); // put buffer in Tristate mode
  state = WAITING;
  OEstate = false;
  nextUpdateTime = millis() + conf.interval * 1000;
}

void P1Reader::ResetnextUpdateTime()
{
  LastSample = millis();
  RTS_off(); // switch off Data Request
}

int P1Reader::FindCharInArray(char array[], char c, int len)
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

/*unsigned int P1Reader::CRC16(unsigned int crc, unsigned char *buf, int len)
{
  for (int pos = 0; pos < len; pos++)
  {
    crc ^= (unsigned int)buf[pos]; // XOR byte into least sig. byte of crc

    for (int i = 8; i != 0; i--) // Loop over each bit
    {
      if ((crc & 0x0001) != 0) // If the LSB is set
      {
        crc >>= 1; // Shift right and XOR 0xA001
        crc ^= 0xA001;
      }
      else // Else LSB is not set
      {
        crc >>= 1; // Just shift right
      }
    }
  }
  return crc;
}*/

String P1Reader::identifyMeter(String Name)
{
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
  //unsigned int currentCRC = 0; // the CRC value of the datagram
  //bool validCRCFound = false;
  
  int startChar = FindCharInArray(telegram, '/', len);
  int endChar = FindCharInArray(telegram, '!', len);

  if (state == WAITING) // we're waiting for a valid start sequence, if this line is not it, just return
  {
    if (startChar >= 0)
    { // start found. Reset CRC calculation
      MainSendDebug("[P1] Start of datagram found");

      //currentCRC = CRC16(0x0000, (unsigned char *)telegram + startChar, len - startChar);
      // reset datagram
      datagram = "";
      datagramValid = false;
      dataEnd = false;
      state = READING;

      nextUpdateTime = millis() + conf.interval * 1000;
      for (int cnt = startChar; cnt < len - startChar; cnt++)
      {
        datagram += telegram[cnt];
      }

      /*if (meterName == "")
      {
        meterName = identifyMeter(meternameSet);
      }*/

      return;
    }
    else
    {
      return; // We're waiting for a valid start char, if we're in the middle of a datagram, just return.
    }
  }

  if (state == READING)
  {
    if (endChar >= 0)
    { // we have found the endchar !
      MainSendDebug("[P1] End of datagram found");
      //state = CHECKSUM;
      // add to crc calc
      dataEnd = true; // we're at the end of the data stream, so mark (for raw data output) We don't know if the data is valid, we will test this below.
                      //  gas22Flag=false;        // assume we have also collected the Gas value
      
      //currentCRC = CRC16(currentCRC, (unsigned char *)telegram + endChar, 1);
      //char messageCRC[4];
      //strncpy(messageCRC, telegram + endChar + 1, 4);
      
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
        state = FAULT;
        return;
      }

      //validCRCFound = (strtol(messageCRC, NULL, 16) == (long)currentCRC);

      //if (validCRCFound)
      //{
        state = DONE;
        datagramValid = true;
        //dataFailureCount = 0;
        LastSample = millis();
        //      gotPowerReading = true; // we at least got electricty readings. Not all setups have a gas meter attached, so gotGasReading is handled when we actually get gasIds coming in
        return;
      //}
      /*else
      {
        MainSendDebug("[P1] INVALID CRC FOUND");
        //dataFailureCount++;
        state = FAILURE;
        return;
      }*/
    }
    else
    { // no endchar, so normal line, process
      //currentCRC = CRC16(currentCRC, (unsigned char *)telegram, len);
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
  case 13028: // device type
    readFirstParenthesisVal(i, len).toCharArray(DataReaded.P1version, sizeof(DataReaded.P1version));
    if (DataReaded.P1version[0] == '4')
    {
      DataReaded.P1prot = 4;
    }
    else
    {
      DataReaded.P1prot = 5;
    }
    break;
  case 100:
    readFirstParenthesisVal(i, len).toCharArray(DataReaded.P1timestamp, sizeof(DataReaded.P1timestamp));
    break;
  case 96140:
    readFirstParenthesisVal(i, len).toCharArray(DataReaded.tariffIndicatorElectricity, sizeof(DataReaded.tariffIndicatorElectricity));
    break;
  case 9611:
    readFirstParenthesisVal(i, len).toCharArray(DataReaded.equipmentId, sizeof(DataReaded.equipmentId));
    break;
  case 19610:
    readFirstParenthesisVal(i, len).toCharArray(DataReaded.equipmentId2, sizeof(DataReaded.equipmentId2));
    break;
  case 9810:
    MainSendDebug("[P1] Disruptions (code 9810)");
    break;
  case 10170: // 1-0:1.7.0 – actualElectricityPowerDelivered
    readUntilStar(i, len).toCharArray(DataReaded.actualElectricityPowerDeli, sizeof(DataReaded.actualElectricityPowerDeli));
    break;
  case 10270: // 1-0:1.7.0 – actualElectricityPowerReturned
    readUntilStar(i, len).toCharArray(DataReaded.actualElectricityPowerRet, sizeof(DataReaded.actualElectricityPowerRet));
    break;
  case 10180: // 1-0:1.8.0 – actualElectricityPowerDelivered
    MainSendDebug("[P1] non existent (code 10180)");
    readUntilStar(i, len).toCharArray(DataReaded.actualElectricityPowerDeli, sizeof(DataReaded.actualElectricityPowerDeli));
    break;
  case 10181: // 1-0:1.8.1(000992.992*kWh) Elektra verbruik laag tarief
    readUntilStar(i, len).toCharArray(DataReaded.electricityUsedTariff1, sizeof(DataReaded.electricityUsedTariff1));
    break;
  case 10182: // 1-0:1.8.2(000560.157*kWh) = Elektra verbruik hoog tarief
    readUntilStar(i, len).toCharArray(DataReaded.electricityUsedTariff2, sizeof(DataReaded.electricityUsedTariff2));
    break;
  case 10281: // 1-0:2.8.1(000348.890*kWh) Elektra opbrengst laag tarief
    readUntilStar(i, len).toCharArray(DataReaded.electricityReturnedTariff1, sizeof(DataReaded.electricityReturnedTariff1));
    break;
  case 10282: // 1-0:2.8.2(000859.885*kWh) Elektra opbrengst hoog tarief
    readUntilStar(i, len).toCharArray(DataReaded.electricityReturnedTariff2, sizeof(DataReaded.electricityReturnedTariff2));
    break;
  case 103170: // 1-0:31.7.0(002*A) Instantane stroom Elektriciteit L1
    readUntilStar(i, len).toCharArray(DataReaded.instantaneousCurrentL1, sizeof(DataReaded.instantaneousCurrentL1));
    break;
  case 105170: // 1-0:51.7.0(002*A) Instantane stroom Elektriciteit L2
    readUntilStar(i, len).toCharArray(DataReaded.instantaneousCurrentL2, sizeof(DataReaded.instantaneousCurrentL2));
    break;
  case 107170: // 1-0:71.7.0(002*A) Instantane stroom Elektriciteit L3
    readUntilStar(i, len).toCharArray(DataReaded.instantaneousCurrentL3, sizeof(DataReaded.instantaneousCurrentL3));
    break;
  case 103270: // 1-0:32.7.0(232.0*V) Voltage L1
    readUntilStar(i, len).toCharArray(DataReaded.instantaneousVoltageL1, sizeof(DataReaded.instantaneousVoltageL1));
    break;
  case 105270: // 1-0:52.7.0(232.0*V) Voltage L2
    readUntilStar(i, len).toCharArray(DataReaded.instantaneousVoltageL2, sizeof(DataReaded.instantaneousVoltageL2));
    break;
  case 107270: // 1-0:72.7.0(232.0*V) Voltage L3
    readUntilStar(i, len).toCharArray(DataReaded.instantaneousVoltageL3, sizeof(DataReaded.instantaneousVoltageL3));
    break;
  case 102170: // 1-0:21.7.0(002*A) Instantane stroom Elektriciteit L1
    readUntilStar(i, len).toCharArray(DataReaded.activePowerL1P, sizeof(DataReaded.activePowerL1P));
    break;
  case 104170: // 1-0:41.7.0(002*A) Instantane stroom Elektriciteit L2
    readUntilStar(i, len).toCharArray(DataReaded.activePowerL2P, sizeof(DataReaded.activePowerL2P));
    break;
  case 106170: // 1-0:61.7.0(002*A) Instantane stroom Elektriciteit L3
    readUntilStar(i, len).toCharArray(DataReaded.activePowerL3P, sizeof(DataReaded.activePowerL3P));
    break;
  case 102270: // 1-0:22.7.0(232.0*V) Voltage L1
    readUntilStar(i, len).toCharArray(DataReaded.activePowerL1NP, sizeof(DataReaded.activePowerL1NP));
    break;
  case 104270: // 1-0:42.7.0(232.0*V) Voltage L2
    readUntilStar(i, len).toCharArray(DataReaded.activePowerL2NP, sizeof(DataReaded.activePowerL2NP));
    break;
  case 106270: // 1-0:62.7.0(232.0*V) Voltage L3
    readUntilStar(i, len).toCharArray(DataReaded.activePowerL3NP, sizeof(DataReaded.activePowerL3NP));
    break;
  case 1032320: // Aantal korte spanningsdalingen Elektriciteit in fase 1
    readFirstParenthesisVal(i, len).toCharArray(DataReaded.numberVoltageSagsL1, sizeof(DataReaded.numberVoltageSagsL1));
    break;
  case 1052320: // Aantal korte spanningsdalingen Elektriciteit in fase 2
    readFirstParenthesisVal(i, len).toCharArray(DataReaded.numberVoltageSagsL2, sizeof(DataReaded.numberVoltageSagsL2));
    break;
  case 1072320: // Aantal korte spanningsdalingen Elektriciteit in fase 3
    readFirstParenthesisVal(i, len).toCharArray(DataReaded.numberVoltageSagsL3, sizeof(DataReaded.numberVoltageSagsL3));
    break;
  case 1032360: // 1-0:32.36.0(00000) Aantal korte spanningsstijgingen Elektriciteit in fase 1
    readFirstParenthesisVal(i, len).toCharArray(DataReaded.numberVoltageSwellsL1, sizeof(DataReaded.numberVoltageSwellsL1));
    break;
  case 1052360: // 1-0:52.36.0(00000) Aantal korte spanningsstijgingen Elektriciteit in fase 2
    readFirstParenthesisVal(i, len).toCharArray(DataReaded.numberVoltageSwellsL2, sizeof(DataReaded.numberVoltageSwellsL2));
    break;
  case 1072360: // 1-0:72.36.0(00000) Aantal korte spanningsstijgingen Elektriciteit in fase 3
    readFirstParenthesisVal(i, len).toCharArray(DataReaded.numberVoltageSwellsL3, sizeof(DataReaded.numberVoltageSwellsL3));
    break;
  case 12421: // gas
    readBetweenDoubleParenthesis(i, len).toCharArray(DataReaded.gasReceived5min, sizeof(DataReaded.gasReceived5min));
    readBetweenDoubleParenthesis(i, len).toCharArray(DataReaded.gasDomoticz, sizeof(DataReaded.gasDomoticz));
    break;
  case 96721: // 0-0:96.7.21(00051)  Number of power failures in any phase
    readFirstParenthesisVal(i, len).toCharArray(DataReaded.numberPowerFailuresAny, sizeof(DataReaded.numberPowerFailuresAny));
    break;
  case 9679: // 0-0:96.7.9(00007) Number of long power failures in any phase
    readFirstParenthesisVal(i, len).toCharArray(DataReaded.numberLongPowerFailuresAny, sizeof(DataReaded.numberLongPowerFailuresAny));
    break;
  case 1099970: // 1-0:99.97.0(6) Power Failure Event Log (long power failures)
    DataReaded.longPowerFailuresLog = "";
    while (i < len)
    {
      DataReaded.longPowerFailuresLog += (char)telegram[i];
      i++;
    }
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
  if ((millis() > nextUpdateTime))
  {
    if (!OEstate)
    {
      RTS_on();
    }
  }

  if (OEstate)
  {
    nextUpdateTime = millis() + conf.interval * 1000;
    OEstate = false;
  }
  readTelegram();
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
        MainSendDebug("[P1] Error, timeout on serial");
        RTS_off();
        return;
      }

      int len = Serial.readBytesUntil('\n', telegram, sizeof(telegram)-2);
      telegram[len] = '\n';
      telegram[len + 1] = 0;
      
      decodeTelegram(len + 1);
      //blink(1, 400);

      switch (state)
      {
      case DISABLED:
        break;
      case WAITING:
        break;
      case READING:
        break;
      //case CHECKSUM:
       // break;
      case DONE:
        RTS_off();
        break;
      case FAILURE:
        // if there is no checksum, (state=Failure && dataEnd)
        MainSendDebug("[P1] kicked out of decode loop (invalid CRC or no CRC!)");
        RTS_off();
        break;
      case FAULT:
        MainSendDebug("[P1] Fault in reading data");
        state = WAITING;
        break;
      default:
        break;
      }
      //yield();
    }
    //digitalWrite(LED_BUILTIN, HIGH);
  }
}
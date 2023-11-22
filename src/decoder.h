/*
 * Copyright (c) 2022 Ronald Leenes
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
 */

/*
      P1_pos = inputString.indexOf("1-0:1.7.0", pos282 + 1);
      P1 = inputString.substring(P1_pos + 10, P1_pos + 17);
*/

/**
 * @file decoder.ino
 * @author Ronald Leenes
 * @date 28.12.2022
 * @version 1.0u
 *
 * @brief This file contains the OBIS parser functions
 *
 * @see http://esp8266thingies.nl
 */

long getValidVal(long valNew, long valOld, long maxDiffer) {
  // check if the incoming value is valid
  if (valOld > 0 &&
      ((valNew - valOld > maxDiffer) && (valOld - valNew > maxDiffer)))
    return valOld;
  return valNew;
}

void getValue(char *theValue, char *buffer, int maxlen, char startchar,
              char endchar) {
  String cleanres = "";
  int s = FindCharInArrayRev(buffer, startchar, maxlen - 2);
  int l = FindCharInArrayRev(buffer, endchar, maxlen - 2) - s - 1;

  char res[16];
  memset(res, 0, sizeof(res));

  if (strncpy(res, buffer + s + 1, l)) {
    if (endchar == '*') {
      if (isNumber(res, l)) {
        int flag = 1;
        for (int i = 0; i < l + 1; i++) {
          if (flag == 1 && res[i] != '0')
            flag = 0;
          if (res[i] == '0' && res[i + 1] == '.')
            flag = 0;
          if (flag != 1) {
            if (!reportInDecimals) { // BELGIQUE // report in Watts instead of
                                     // KW
              if (res[i] != '.')
                cleanres += res[i];
            } else
              cleanres += res[i];
          }
        }
      }
      if (cleanres == "")
        cleanres = "0"; // make sure there is a value to be be returned. 1.1.bea
                        // -19 Aug

      cleanres.toCharArray(theValue, cleanres.length());
      theValue[cleanres.length() + 1] = 0;
    } else if (endchar == ')') {
      if (isNumber(res, l))
        strncpy(theValue, res, l);
      theValue[cleanres.length() + 1] = 0;
    }
  }
}

void getGasValue(char *theValue, char *buffer, int maxlen, char startchar,
                 char endchar) {
  String cleanres = "";
  bool nodecimals = false;

  if (!reportInDecimals)
    nodecimals = true;
  // if (domoticzJson) nodecimals = true;

  int s = 0;
  if (FindCharInArrayRev2(buffer, ')', maxlen - 2) !=
      -1) // some meters report the meterID in () before the section with actual
          // gas value
    s = FindCharInArrayRev2(buffer, ')', maxlen - 2) + 1;
  else
    s = FindCharInArrayRev(buffer, '(', maxlen - 2);

  if (s < 8)
    return;
  if (s > 32)
    s = 32;
  int l = FindCharInArrayRev(buffer, '*', maxlen - 2) - s - 1;
  if (l < 4)
    return;
  if (l > 12)
    return;
  char res[16];
  memset(res, 0, sizeof(res));
  if (strncpy(res, buffer + s + 1, l)) {
    if (isNumber(res, l)) {
      int flag = 1;
      for (int i = 0; i < l + 1; i++) {
        if (flag == 1 && res[i] != '0')
          flag = 0;
        if (res[i] == '0' && res[i + 1] == '.')
          flag = 0;
        if (flag != 1) {
          if (nodecimals) {
            if (res[i] != '.')
              cleanres += res[i];
          } else
            cleanres += res[i];
        }
      }
    }
    cleanres.toCharArray(theValue, cleanres.length());
    theValue[cleanres.length() + 1] = 0;
  }
}

void getGas22Value(char *theValue, char *buffer, int maxlen, char startchar,
                   char endchar) {
  int s = FindCharInArrayRev(buffer, startchar, maxlen - 2);
  int l = FindCharInArrayRev(buffer, endchar, maxlen - 2) - s - 1;
  char res[16];
  memset(res, 0, sizeof(res));

  if (strncpy(res, buffer + s + 1, l)) {
    if (isNumber(res, l))
      strncpy(theValue, res, l);
    theValue[l + 1] = 0;
  }
}

void getDomoticzGasValue(char *theValue, char *buffer, int maxlen,
                         char startchar, char endchar) {
  String cleanres = "";
  bool nodecimals = false;

  if (!reportInDecimals)
    nodecimals = true;

  int s = 0;
  if (FindCharInArrayRev2(buffer, ')', maxlen - 2) !=
      -1) // some meters report the meterID in () before the section with actual
          // gas value
    s = FindCharInArrayRev2(buffer, ')', maxlen - 2) + 1;
  else
    s = FindCharInArrayRev(buffer, '(', maxlen - 2);

  if (s < 8)
    return;
  if (s > 32)
    s = 32;
  int l = FindCharInArrayRev(buffer, '*', maxlen - 2) - s - 1;
  if (l < 4)
    return;
  if (l > 12)
    return;
  char res[16];
  memset(res, 0, sizeof(res));
  if (strncpy(res, buffer + s + 1, l)) {
    //    if (endchar == '*')
    //   {
    if (isNumber(res, l)) {
      int flag = 1;
      for (int i = 0; i < l + 1; i++) {
        if (flag == 1 && res[i] != '0')
          flag = 0;
        if (res[i] == '0' && res[i + 1] == '.')
          flag = 0;
        if (flag != 1) {
          if (res[i] != '.')
            cleanres += res[i];
        }
      }
    }
    cleanres.toCharArray(theValue, cleanres.length());
    theValue[cleanres.length() + 1] = 0;
    //  } else if (endchar == ')')  if (isNumber(res, l))  strncpy(theValue,
    //  res, l);
  }
}

void getStr(char *theValue, char *buffer, int maxlen, char startchar,
            char endchar) {
  int s = FindCharInArrayRev(buffer, startchar, maxlen - 2);
  int l = FindCharInArrayRev(buffer, endchar, maxlen - 2) - s - 1;
  char res[102];
  memset(res, 0, sizeof(res));

  if (strncpy(res, buffer + s + 1, l)) {
    if (isNumber(res, l))
      strncpy(theValue, res, l);
    theValue[l + 1] = 0;
  }
}

void getStr12(char *theValue, char *buffer, int maxlen, char startchar) {
  int s = FindCharInArrayRev(buffer, startchar, maxlen - 2);
  int l = 12; // FindCharInArrayRev(buffer, endchar, maxlen - 2) - s - 1;
  char res[102];
  memset(res, 0, sizeof(res));

  if (strncpy(res, buffer + s + 1, l)) {
    if (isNumber(res, l))
      strncpy(theValue, res, l);
    theValue[l + 1] = 0;
  }
}

bool decodeTelegram(int len) {
  long val = 0;
  long val2 = 0;
  int pos4;
  // need to check for start
  int startChar = FindCharInArrayRev(telegram, '/', len);
  int endChar = FindCharInArrayRev(telegram, '!', len);
  bool validCRCFound = false;
  char payload[50];

  if (state == WAITING) { // we're waiting for a valid start sequence, if this
                          // line is not it, just return
    if (startChar >= 0) {

      // start found. Reset CRC calculation
      currentCRC =
          CRC16(0x0000, (unsigned char *)telegram + startChar, len - startChar);
      // and reset datagram
      datagram = "";
      datagramValid = false;
      dataEnd = false;

      for (int cnt = startChar; cnt < len - startChar; cnt++) {
        // debug(telegram[cnt]);
        datagram += telegram[cnt];
        if (devicestate == CONFIG)
          meterId += telegram[cnt]; // we only need to collect the meterId once
      }
      debugln("Start found!");
      if (devicestate == CONFIG)
        identifyMeter();
      state = READING;
      return false;
    } else {
      currentCRC = 0;
      return false; // We're waiting for a valid start char, if we're in the
                    // middle of a datagram, just return.
    }
  }

  if (state == READING) {
    if (endChar >= 0) // we have found the endchar !
    {
      nextUpdateTime = millis() + interval; // set trigger for next round

      state = CHECKSUM;
      // add to crc calc
      dataEnd = true; // we're at the end of the data stream, so mark (for raw
                      // data output) We don't know if the data is valid, we
                      // will test this below.
      gas22Flag = false; // assume we have also collected the Gas value
      currentCRC = CRC16(currentCRC, (unsigned char *)telegram + endChar, 1);
      char messageCRC[4];
      strncpy(messageCRC, telegram + endChar + 1, 4);
      if (datagram.length() < 2048) {
        for (int cnt = 0; cnt < len; cnt++) {
          datagram += telegram[cnt];
        }
        datagram += "\r";
        datagram += "\n";
      } else
        datagram = ""; // prevent bufferoverflow

      validCRCFound = (strtol(messageCRC, NULL, 16) == currentCRC);
      // debug("   calculated CRC:");
      // debugln(currentCRC);

      if (validCRCFound) {
        debugln("\nVALID CRC FOUND!");
        LastSample = millis();
        datagramValid = true;
        gotPowerReading =
            true; // we at least got electricty readings. Not all setups have a
                  // gas meter attached, so gotGasReading is handled when we
                  // actually get gasIds coming in
        state = DONE;
        RTS_off();
        if (devicestate == GOTMETER) {
          devicestate = RUNNING;
        }

        return true;
      } else {
        debugln("\n===INVALID CRC FOUND!===");
        state = FAILURE;
        currentCRC = 0;
        RTS_off();
        return false;
      }
    }

    else { // normal line, process
      currentCRC = CRC16(currentCRC, (unsigned char *)telegram, len);
      for (int cnt = 0; cnt < len; cnt++) {
        datagram += telegram[cnt];
      }

      if ((telegram[4] >= '0') && (telegram[4] <= '9'))
        pos4 = (int)(telegram[4]) - 48;
      else
        pos4 = 10;

      if (devicestate == CONFIG) {
        // 0-0:96.1.1 equipmentId                         (xxxxxxxxxxxx)
        if (strncmp(telegram, "96.1.0", strlen("96.1.0")) == 0) {
          getStr(equipmentId, telegram, len, '(', ')');
          devicestate = GOTMETER;
        }
        // 1-3:0.2.8(42) or 1-3:0.2.8(50) // protocol version
        if (strncmp(telegram, "1-3:0.2.8", strlen("1-3:0.2.8")) == 0) {
          getStr(P1version, telegram, len, '(', ')');
          if (P1version[0] == '4')
            P1prot = 4;
          else
            P1prot = 5;
        }
      }
      // debugln(pos4);
      debug2(">>>>>>>telegram: ");
      debug2ln(telegram);

      switch (pos4) {
      case 1:
        // 0-0:1.0.0.255 datestamp YYMMDDhhmmssX
        if (strncmp(telegram, "0-0:1.0.0", strlen("0-0:1.0.0")) == 0) {
          getStr12(P1timestamp, telegram, len, '(');
          if (timeStatus() == timeNotSet)
            settime();
          timeIsSet = true;
          if ((hour() == 0) && (minute() == 30))
            settime(); // resync every 24 hours
          break;
        }

        // 1-0:1.7.0(00.424*kW) Actueel verbruik
        // 1-0:1.7.x = Electricity consumption current usage
        if (strncmp(telegram, "1-0:1.7.0", strlen("1-0:1.7.0")) == 0) {
          getValue(actualElectricityPowerDeli, telegram, len, '(', '*');
          sprintf(payload, "1-0:1.7.0", actualElectricityPowerRet);
          if (MQTT_debug)
            send_mqtt_message("p1wifi/logging", payload);
          debug2("case 1: actualElectricityPowerDeli: ");
          debug2ln(actualElectricityPowerDeli);
          break;
        }
        // 1-0:1.8.1(000992.992*kWh) Elektra verbruik laag tarief
        if (strncmp(telegram, "1-0:1.8.1", strlen("1-0:1.8.1")) == 0) {
          getValue(electricityUsedTariff1, telegram, len, '(', '*');
          break;
        }

        // 1-0:1.8.2(000560.157*kWh) = Elektra verbruik hoog tarief
        if (strncmp(telegram, "1-0:1.8.2", strlen("1-0:1.8.2")) == 0) {
          getValue(electricityUsedTariff2, telegram, len, '(', '*');
          break;
        }

      case 2:
        if (strncmp(telegram, "1-0:2.7.0", strlen("1-0:2.7.0")) == 0) {
          getValue(actualElectricityPowerRet, telegram, len, '(', '*');
          debug2("decoder case 2: actualElectricityPowerRet: ");
          debug2ln(actualElectricityPowerRet);
          break;
        }
        // 1-0:21.7.0(00.378*kW) Instantaan vermogen Elektriciteit levering L1
        if (strncmp(telegram, "1-0:21.7.0", strlen("1-0:21.7.0")) == 0) {
          getValue(activePowerL1P, telegram, len, '(', '*');
          break;
        }

        // 1-0:22.7.0(00.378*kW) Instantaan vermogen Elektriciteit levering L1
        if (strncmp(telegram, "1-0:21.7.0", strlen("1-0:21.7.0")) == 0) {
          getValue(activePowerL1NP, telegram, len, '(', '*');
          break;
        }

        //        if (meterId.indexOf("ISK5\2M550E-1011") ==0){
        //          debugln("ISKRA exception");
        //            if (strncmp(telegram, "1-0:2.7.0", strlen("1-0:2.7.0")) ==
        //            0)
        //                getValue(actualElectricityPowerDeli, telegram, len,
        //                '(', '*'); // kludge for ISKRA meter
        //                // 1-0:22.7.0(00.378*kW) Instantaan vermogen
        //                Elektriciteit levering L1
        //            if (strncmp(telegram, "1-0:22.7.0", strlen("1-0:22.7.0"))
        //            == 0)
        //                getValue(actualElectricityPowerRet, telegram, len,
        //                '(', '*'); // kludge for ISKRA meter
        //        }

        // 1-0:2.8.1(000348.890*kWh) Elektra opbrengst laag tarief
        if (strncmp(telegram, "1-0:2.8.1", strlen("1-0:2.8.1")) == 0) {
          getValue(electricityReturnedTariff1, telegram, len, '(', '*');
          break;
        }

        // 1-0:2.8.2(000859.885*kWh) Elektra opbrengst hoog tarief
        if (strncmp(telegram, "1-0:2.8.2", strlen("1-0:2.8.2")) == 0) {
          getValue(electricityReturnedTariff2, telegram, len, '(', '*');
          break;
        }

        // 1-0:21.7.0(00.378*kW) Instantaan vermogen Elektriciteit levering L1
        if (strncmp(telegram, "1-0:21.7.0", strlen("1-0:21.7.0")) == 0) {
          getValue(activePowerL1P, telegram, len, '(', '*');
          break;
        }

        // 1-0:22.7.0(00.378*kW) Instantaan vermogen Elektriciteit retour L1
        if (strncmp(telegram, "1-0:22.7.0", strlen("1-0:22.7.0")) == 0) {
          getValue(activePowerL1NP, telegram, len, '(', '*');
          break;
        }

        ////          // 0-1:24.2.1(150531200000S)(00811.923*m3)
        ////          // 0-1:24.2.1 = Gas (DSMR v4.0) on Kaifa MA105 meter,
        ///other meters do (number)(gas value)
        if (strncmp(telegram, "0-1:24.2.1", strlen("0-1:24.2.1")) == 0) {
          getGasValue(gasReceived5min, telegram, len, '(', ')');
          //      break;
        }
        ////
        ////         // 0-1:24.2.1(150531200000S)(00811.923*m3)
        ////        // 0-1:24.2.1 = Gas (DSMR v4.0) on Kaifa MA105 meter, other
        ///meters do (number)(gas value)
        if (strncmp(telegram, "0-1:24.2.1", strlen("0-1:24.2.1")) == 0) {
          getDomoticzGasValue(gasDomoticz, telegram, len, '(', ')');
          gotGasReading = true;
          break;
        }

        break;

      case 3:

        // 1-0:31.7.0(002*A) Instantane stroom Elektriciteit L1
        if (strncmp(telegram, "1-0:31.7.0", strlen("1-0:31.7.0")) == 0) {
          getValue(instantaneousCurrentL1, telegram, len, '(', '*');
          break;
        }

        // 1-0:32.7.0(232.0*V) Voltage L1
        if (strncmp(telegram, "1-0:32.7.0", strlen("1-0:32.7.0")) == 0) {
          getStr(instantaneousVoltageL1, telegram, len, '(', '*');
          break;
        }

        // 1-0:32.32.0(00000) Aantal korte spanningsdalingen Elektriciteit in
        // fase 1
        if (strncmp(telegram, "1-0:32.32.0", strlen("1-0:32.32.0")) == 0) {
          getValue(numberVoltageSagsL1, telegram, len, '(', ')');
          break;
        }

        // 1-0:32.36.0(00000) Aantal korte spanningsstijgingen Elektriciteit in
        // fase 1
        if (strncmp(telegram, "1-0:32.36.0", strlen("1-0:32.36.0")) == 0) {
          getValue(numberVoltageSwellsL1, telegram, len, '(', ')');
          break;
        }
        break;

      case 4:

        // 1-0:41.7.0(00.378*kW) Instantaan vermogen Elektriciteit levering L2
        if (strncmp(telegram, "1-0:41.7.0", strlen("1-0:41.7.0")) == 0) {
          getValue(activePowerL2P, telegram, len, '(', '*');
          break;
        }

        // 1-0:42.7.0(00.378*kW) Instantaan vermogen Elektriciteit levering L2
        if (strncmp(telegram, "1-0:41.7.0", strlen("1-0:41.7.0")) == 0) {
          getValue(activePowerL2NP, telegram, len, '(', '*');
          break;
        }

        // 1-0:42.7.0(00.378*kW) Instantaan vermogen Elektriciteit retour L2
        if (strncmp(telegram, "1-0:42.7.0", strlen("1-0:42.7.0")) == 0) {
          getValue(activePowerL2NP, telegram, len, '(', '*');
          break;
        }
        break;

      case 5:
        // 1-0:52.32.0(00000) voltage sags L1
        if (strncmp(telegram, "1-0:52.32.0", strlen("1-0:52.32.0")) == 0) {
          getValue(numberVoltageSagsL1, telegram, len, '(', ')');
          break;
        }

        // 1-0:52.36.0(00000) voltage swells L1
        if (strncmp(telegram, "1-0:52.36.0", strlen("1-0:52.36.0")) == 0) {
          getValue(numberVoltageSwellsL1, telegram, len, '(', ')');
          break;
        }

        // 1-0:51.7.0(002*A) Instantane stroom Elektriciteit L2
        if (strncmp(telegram, "1-0:51.7.0", strlen("1-0:51.7.0")) == 0) {
          getValue(instantaneousCurrentL2, telegram, len, '(', '*');
          break;
        }

        // 1-0:52.7.0(232.0*V) Voltage L2
        if (strncmp(telegram, "1-0:52.7.0", strlen("1-0:52.7.0")) == 0) {
          getStr(instantaneousVoltageL2, telegram, len, '(', '*');
          break;
        }

        break;

      case 6:
        // 1-0:61.7.0(00.378*kW) Instantaan vermogen Elektriciteit levering L3
        if (strncmp(telegram, "1-0:61.7.0", strlen("1-0:61.7.0")) == 0) {
          getValue(activePowerL3P, telegram, len, '(', '*');
          break;
        }

        // 1-0:62.7.0(00.378*kW) Instantaan vermogen Elektriciteit retour L3
        if (strncmp(telegram, "1-0:62.7.0", strlen("1-0:62.7.0")) == 0) {
          getValue(activePowerL3NP, telegram, len, '(', '*');
          break;
        }

        break;

      case 7:
        // 1-0:71.7.0(002*A) Instantane stroom Elektriciteit L3
        if (strncmp(telegram, "1-0:71.7.0", strlen("1-0:71.7.0")) == 0) {
          getValue(instantaneousCurrentL3, telegram, len, '(', '*');
          break;
        }

        // 1-0:72.7.0(232.0*V) Voltage L3
        if (strncmp(telegram, "1-0:72.7.0", strlen("1-0:72.7.0")) == 0) {
          getStr(instantaneousVoltageL3, telegram, len, '(', '*');
          break;
        }

        // 1-0:72.32.0(00000) voltage sags L3
        if (strncmp(telegram, "1-0:72.32.0", strlen("1-0:72.32.0")) == 0) {
          getValue(numberVoltageSagsL3, telegram, len, '(', ')');
          break;
        }

        // 1-0:72.36.0(00000) voltage swells L3
        if (strncmp(telegram, "1-0:72.36.0", strlen("1-0:72.36.0")) == 0) {
          getValue(numberVoltageSwellsL3, telegram, len, '(', ')');
          break;
        }

        break;

      case 9:
        // 0-0:96.14.0(0001) Actual Tarif
        if (strncmp(telegram, "0-0:96.14.0", strlen("0-0:96.14.0")) == 0) {
          getStr(tariffIndicatorElectricity, telegram, len, '(', ')');
          break;
        }

        // 0-0:96.7.21(00003) Aantal onderbrekingen Elektriciteit
        if (strncmp(telegram, "0-0:96.7.21", strlen("0-0:96.7.21")) == 0) {
          getStr(numberPowerFailuresAny, telegram, len, '(', ')');
          break;
        }

        // 0-0:96.7.9(00001) Aantal lange onderbrekingen Elektriciteit
        if (strncmp(telegram, "0-0:96.7.9", strlen("0-0:96.7.9")) == 0) {
          getStr(numberLongPowerFailuresAny, telegram, len, '(', ')');
          break;
        }

        if (strncmp(telegram, "0-1:96.1.1", strlen("0-1:96.1.1")) == 0) {
          getStr(equipmentId, telegram, len, '(', ')');
          break;
        }

        break;

      default:
        break;
      }
    }
    return validCRCFound; // true if valid CRC found

  } // state = reading
  return false;
}

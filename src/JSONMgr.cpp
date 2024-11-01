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

#include "JSONMgr.h"

JSONMgr::JSONMgr(settings &configuration, P1Reader &currentP1) : conf(configuration), P1Captor(currentP1)
{
  P1Captor.OnNewDatagram([this]()
  {
    UpdateElectricity();
    UpdateGas();
  });
}

void JSONMgr::UpdateGas()
{
  DomoticzJson(conf.domoticzGasIdx, 0, P1Captor.DataReaded.gasDomoticz);
}

/// @brief sends the electricity usage to server
void JSONMgr::UpdateElectricity()
{
  char sValue[300];
  sprintf(sValue, "%f;%f;%f;%f;%f;%f", P1Captor.DataReaded.electricityUsedTariff1.val(), P1Captor.DataReaded.electricityUsedTariff2.val(), P1Captor.DataReaded.electricityReturnedTariff1.val(), P1Captor.DataReaded.electricityReturnedTariff2.val(), P1Captor.DataReaded.actualElectricityPowerDeli.val(), P1Captor.DataReaded.actualElectricityPowerRet.val());
  DomoticzJson(conf.domoticzEnergyIdx, 0, sValue);
}

/// @brief Send to Domoticz data
/// @param idx 
/// @param nValue 
/// @param sValue 
void JSONMgr::DomoticzJson(unsigned int idx, int nValue, char* sValue)
{
  WiFiClient client;
  HTTPClient http;
  
  if (conf.domo)
  {
    char url[255];
    sprintf(url, "http://%s:%u/json.htm?type=command&param=udevice&idx=%u&nvalue=%d&svalue=%s", conf.domoticzIP, conf.domoticzPort, idx, nValue, sValue);
    MainSendDebugPrintf("[HTTP] Send GET : %s", url);
    
    http.begin(client, url);
    int httpCode = http.GET();
    
    // httpCode will be negative on error
    if (httpCode > 0)
    { // HTTP header has been sent and Server response header has been handled

      if (httpCode == HTTP_CODE_OK)
      {
        String payload = http.getString();
      }
    }
    else
    {
      MainSendDebugPrintf("[HTTP] GET failed, error: %s", http.errorToString(httpCode).c_str());
    }
    http.end();
  }
}
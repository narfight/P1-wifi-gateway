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

#include "DomoticzMgr.h"

DomoticzMgr::DomoticzMgr(settings &configuration, P1Reader &currentP1) : conf(configuration), P1Captor(currentP1)
{
  P1Captor.OnNewDatagram([this]()
  {
    UpdateElectricity();
    UpdateGas();
  });
}

void DomoticzMgr::UpdateGas()
{
  if (conf.domoticzGasIdx == 0)
  {
    return;
  }

  MainSendDebugPrintf("[DMTCZ] Send Gas");
  SendToDomoticz(conf.domoticzGasIdx, P1Captor.DataReaded.gasDomoticz, false);
}

void DomoticzMgr::UpdateElectricity()
{
  if (conf.domoticzEnergyIdx == 0)
  {
    return;
  }

  MainSendDebugPrintf("[DMTCZ] Send Gas");
  char sValue[300];
  sprintf(sValue, "%f;%f;%f;%f;%f;%f", P1Captor.DataReaded.electricityUsedTariff1.val(), P1Captor.DataReaded.electricityUsedTariff2.val(), P1Captor.DataReaded.electricityReturnedTariff1.val(), P1Captor.DataReaded.electricityReturnedTariff2.val(), P1Captor.DataReaded.actualElectricityPowerDeli.val(), P1Captor.DataReaded.actualElectricityPowerRet.val());
  SendToDomoticz(conf.domoticzEnergyIdx, sValue, false);
}

void DomoticzMgr::SendDebug(const char *payload)
{
  if (conf.domo && conf.debugToDomo)
  {
    SendToDomoticz(conf.domoticzDebugIdx, payload, true);
  }
}

int DomoticzMgr::SendToDomoticz(unsigned int idx, const char* sValue, bool DisableDebug)
{
  WiFiClient client;
  HTTPClient http;
  
  char url[255];
  sprintf(url, "http://%s:%u/json.htm?type=command&param=udevice&idx=%u&svalue=%s", conf.domoticzIP, conf.domoticzPort, idx, sValue);
  MainSendDebugPrintf("[DMTCZ] Send data : %s", url);
  
  http.begin(client, url);
  int httpCode = http.GET();
  
  // httpCode will be negative on error
  if (httpCode < 0 && !DisableDebug)
  {
    MainSendDebugPrintf("[DMTCZ] GET failed, error: %s", http.errorToString(httpCode).c_str());
  }
  http.end();
  return httpCode;
}
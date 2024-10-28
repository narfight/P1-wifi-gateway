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

#include "WifiMgr.h"

void WifiMgr::DoMe()
{
  if (WiFi.status() != LastStatusEvent)
  {
    wl_status_t tmp = LastStatusEvent;
    LastStatusEvent = WiFi.status();
    
    MainSendDebugPrintf("[WIFI][Connected:%s] Event %s -> %s", (WiFi.isConnected())? "Y" : "N", StatusIdToString(tmp).c_str(), StatusIdToString(LastStatusEvent).c_str());

    if (LastStatusEvent == WL_NO_SSID_AVAIL)
    {
      Connect(); // on relance les tentatives
      return;
    }

    if (DelegateWifiChange != nullptr)
    { // Que si quelqu'un ecoute l'event
      DelegateWifiChange(WiFi.isConnected(), tmp, LastStatusEvent);
    }
  }

  if (AsAP() && (millis() - LastScanSSID) > INTERVAL_SCAN_SSID_MS && conf.ssid[0] != '\0')
  {
    LastScanSSID = millis();
    //en mode AP, scan les SSID pour savoir si il peut se connecter seulement si un SSID est donné
    if (FindThesSSID())
    {
      Reconnect();
    }
  }
}
String WifiMgr::CurrentIP()
{
  return WiFi.localIP().toString();
}

bool WifiMgr::FindThesSSID()
{
  MainSendDebugPrintf("[WIFI] Looking for : %s", conf.ssid);
  
  // Scan des réseaux WiFi
  int numNetworks = WiFi.scanNetworks();

  if (numNetworks == 0)
  {
    return false;
  }

  // Parcourir la liste des réseaux WiFi
  for (int i = 0; i < numNetworks; ++i)
  {
    // Vérifier si le réseau recherché est présent
    if (strcmp(WiFi.SSID(i).c_str(), conf.ssid) == 0)
    {
      return true;
    }
  }

  return false;
}


/// @brief Traduit l'id de status en string
/// @param status
/// @return
String WifiMgr::StatusIdToString(wl_status_t status)
{
  switch (status)
  {
  case WL_NO_SHIELD:
    return "NO_SHIELD";
    break;
  case WL_IDLE_STATUS:
    return "IDLE_STATUS";
    break;
  case WL_NO_SSID_AVAIL:
    return "NO_SSID_AVAIL";
    break;
  case WL_SCAN_COMPLETED:
    return "SCAN_COMPLETED";
    break;
  case WL_CONNECTED:
    return "CONNECTED";
    break;
  case WL_CONNECT_FAILED:
    return "CONNECT_FAILED";
    break;
  case WL_CONNECTION_LOST:
    return "CONNECTION_LOST";
    break;
  case WL_WRONG_PASSWORD:
    return "WRONG_PASSWORD";
    break;
  case WL_DISCONNECTED:
    return "DISCONNECTED";
    break;
  default:
    return "UNKNOW";
    break;
  }
}

WifiMgr::WifiMgr(settings &currentConf) : conf(currentConf)
{
  LastStatusEvent = WL_IDLE_STATUS;
  WiFi.persistent(true);
  WiFi.setAutoConnect(true);
  WiFi.setAutoReconnect(true);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
}

void WifiMgr::OnWifiEvent(std::function<void(bool, wl_status_t, wl_status_t)> CallBack)
{
  DelegateWifiChange = CallBack;
}


void WifiMgr::Connect()
{
  if (strcmp(conf.ssid, "") != 0)
  {
    // Wifi configured to connect to one wifi
    MainSendDebugPrintf("[WIFI] Connect to '%s'", conf.ssid);
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    WiFi.begin(conf.ssid, conf.password);

    byte tries = 0;
    while (WiFi.status() != WL_CONNECTED)
    {
      digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
      Yield_Delay(300);

      if (tries++ > 30)
      {
        SetAPMod();
        break;
      }
    }

    // if connected to wifi's user
    if (WiFi.getMode() == WIFI_STA && WiFi.status() == WL_CONNECTED)
    {
      WiFi.onStationModeDisconnected([](const WiFiEventStationModeDisconnected &event)
      {
        MainSendDebugPrintf("[WIFI] Perte de communication : %s", event.reason);
      });
      WiFi.setAutoReconnect(true);
      MainSendDebugPrintf("[WIFI] Running, IP : %s", WiFi.localIP().toString());
      setRFPower();
      digitalWrite(LED_BUILTIN, LED_OFF);
    }
  }
  else
  {
    SetAPMod();
  }
}

void WifiMgr::Reconnect()
{
  MainSendDebugPrintf("[WIFI] Trying to Reconnect to '%s' wifi network", conf.ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(conf.ssid, conf.password);
  byte tries = 0;
  while (WiFi.status() != WL_CONNECTED)
  {
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    Yield_Delay(300);
    if (tries++ < 30)
    {
      MainSendDebugPrintf("[WIFI] Can't connect to '%s' !", conf.ssid);
      SetAPMod();
      return;
    }
  }
}

bool WifiMgr::IsConnected()
{
  return WiFi.isConnected();
}
void WifiMgr::SetAPMod()
{
  digitalWrite(LED_BUILTIN, LED_ON);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(GetClientName(), "");
  MainSendDebugPrintf("[WIFI] Start captive Portal '%s' (IP:%s)", GetClientName(), WiFi.softAPIP().toString().c_str());
  APtimer = millis();
}

/// @brief Si il diffuse son propre AP et non connecté a un Wifi
/// @return True si c'est en mode AP
bool WifiMgr::AsAP()
{
  return (WiFi.getMode() != WIFI_STA);
}

void WifiMgr::setRFPower()
{
  float newPower = CalcuAdjustWiFiPower();
  
  if (newPower == -1)
  {
    return; //not connected or error
  }

  // N'applique le changement que si la différence est significative
  if (currentPowerWifi == -1 || abs(currentPowerWifi - newPower) >= HYSTERESIS_CORRECTION_POWER)
  {
    WiFi.setOutputPower(currentPowerWifi);
    currentPowerWifi = newPower;
  }
}

float WifiMgr::CalcuAdjustWiFiPower()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    return -1;
  }

  int rssi = WiFi.RSSI();
  float newPower;
  
  // Protection contre les valeurs RSSI invalides
  if (rssi >= 0)
  {
    return -1;
  }
  
  if (rssi > -50)
  {
    // Signal fort : puissance minimale
    newPower = 0;
  }
  else if (rssi < -80)
  {
    // Signal faible : puissance maximale
    newPower = 20.5;
  }
  else 
  {
    // Entre -50 et -80 : adaptation linéaire
    // Convertit -80~-50 en 0~20.5
    newPower = ((-rssi - 50) * 20.5) / 30;
  }
  
  return newPower;
}
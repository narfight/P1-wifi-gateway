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

#ifndef WIFIMGR
#define WIFIMGR

#define SSID_SETUP "P1_setup_"
#define INTERVAL_SCAN_SSID_MS 180000
#define HYSTERESIS_CORRECTION_POWER 2.0

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "GlobalVar.h"
#include "Debug.h"

// van ESP8266WiFi/examples/WiFiShutdown/WiFiShutdown.ino
#ifndef RTC_config_data_SLOT_WIFI_STATE
#define RTC_config_data_SLOT_WIFI_STATE 33u
#endif

/// @brief everything related to WIFI
class WifiMgr
{
  private:
  settings& conf;
  std::function<void(bool, wl_status_t, wl_status_t)> DelegateWifiChange;
  wl_status_t LastStatusEvent;
  void SetAPMod();
  bool FindThesSSID();
  float CalcuAdjustWiFiPower();
  unsigned long LastScanSSID = millis(); //Last time when the scan of SSID was do
  unsigned long OfflineSince = 0; //Number of ms since wifi don't connected before switch to AP
  float currentPowerWifi = -1; //Power of Wifi TX interface
  public:
  explicit WifiMgr(settings& currentConf);
  unsigned long APtimer = 0; // time we went into AP mode. Restart module if in AP for 10 mins as we might have gotten in AP due to router wifi issue
  WiFiClient WifiCom;
  void DoMe();
  String StatusIdToString(wl_status_t status);
  String CurrentIP();
  void OnWifiEvent(std::function<void(bool, wl_status_t, wl_status_t)> CallBack);
  bool IsConnected();
  void Connect();

  /// @brief Si il diffuse son propre AP et non connecté a un Wifi
  /// @return True si c'est en mode AP
  bool AsAP();

  void setRFPower();
  void Reconnect();
};
#endif
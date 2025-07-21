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

#ifndef VARMGR_H
#define VARMGR_H

#include <Arduino.h>

#define VERSION "1.03"
#define HOSTNAME "P1meter"

#define CLIENTNAMESIZE 32

#define OE 16 // IO16 OE on the 74AHCT1G125
#define DR 4  // IO4 is Data Request

#define LED_ON 0x0
#define LED_OFF 0x1

#define SETTINGVERSIONNULL 0 //= no config
#define SETTINGVERSION 3

struct settings
{
  byte ConfigVersion;
  byte BootFailed = 0;
  bool NeedConfig = true;
  char ssid[33];
  char password[65];
  char domoticzIP[30];
  unsigned int domoticzPort;
  unsigned int domoticzEnergyIdx;
  unsigned int domoticzGasIdx;
  char mqttTopic[50];
  char mqttIP[30];
  unsigned int mqttPort;
  char mqttUser[32];
  char mqttPass[32];
  unsigned int interval;
  bool domo = true;
  bool mqtt = false;
  bool InverseHigh_1_2_Tarif = false;
  bool telnet = false;
  bool debugToMqtt = false;
  bool debugToTelnet = false;
  char adminPassword[33];
  char adminUser[33];
  bool Repport2Telnet;
  bool debugToDomo = true;
  unsigned int domoticzDebugIdx;
};

#ifndef LANGUAGE
  #error "LANGUAGE n'est pas défini. Veuillez définir LANGUAGE dans GlobalVar.h"
#endif

#include "Langues/1-French.h"
#include "Langues/2-English.h"
#include "Langues/3-Dutch.h"

#ifndef LANG_LOADED
  #error "LANGUAGE n'a pas une valeur valide"
#endif
#endif
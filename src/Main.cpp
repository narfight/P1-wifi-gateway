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

#define FRENCH // NEDERLANDS,SWEDISH,GERMAN,FRENCH
#define DEBUG

#define MAXBOOTFAILURE 3 //reset setting if boot fail more than this

#include <Arduino.h>
#include <EEPROM.h>
#include "GlobalVar.h"
#include "Language.h"

unsigned long WatchDogsTimer = 0;

settings config_data;

#include "WifiMgr.h"
WifiMgr *WifiClient;

#include "MQTT.h"
MQTTMgr *MQTTClient;

#include "TelnetMgr.h"
TelnetMgr *TelnetServer;

#include "P1Reader.h"
P1Reader *DataReaderP1;

#include "JSONMgr.h"
JSONMgr *JSONClient;

#include "HTTPMgr.h"
HTTPMgr *HTTPClient;

ADC_MODE(ADC_VCC); // allows you to monitor the internal VCC level;

void MainSendDebug(String payload)
{
  if (MQTTClient != nullptr)
  {
    MQTTClient->SendDebug(payload);
  }
  if (TelnetServer != nullptr)
  {
    TelnetServer->SendDebug(payload);
  }
  #ifdef DEBUG
  Serial.println(payload);
  #endif
}

void MainSendDebugPrintf(const char *format, ...)
{
  const int bufferSize = 100; // Définir la taille du tampon pour stocker le message formaté
  char buffer[bufferSize];

  // Utiliser la liste d'arguments variables
  va_list args;
  va_start(args, format);

  // Formater la chaîne avec vsnprintf
  int length = vsnprintf(buffer, bufferSize, format, args);

  va_end(args);

  // Vérifier si le formatage a réussi et imprimer le message
  if (length >= 0 && length < bufferSize)
  {
    MainSendDebug(buffer);
  }
  else
  {
    MainSendDebug("Erreur de formatage du message de débogage.");
  }
}

void EventOnWifi(bool Connected, wl_status_t from, wl_status_t to)
{
  MainSendDebugPrintf("[WIFI][Connected:%s] Event %s -> %s", (WiFi.isConnected())? "Y" : "N", WifiClient->StatusIdToString(from).c_str(), WifiClient->StatusIdToString(to).c_str());
}

void blink(int t, unsigned long speed)
{
  for (int i = 0; i <= t; i++)
  {
    digitalWrite(LED_BUILTIN, LOW);
    delay(speed);
    digitalWrite(LED_BUILTIN, HIGH);
  }
}

void alignToTelegram()
{
  // make sure we don't drop into the middle of a telegram on boot. Read whatever is in the stream until we find the end char !
  // then read until EOL and flsuh serial, return to loop to pick up the first complete telegram.

  if (Serial.available() > 0)
  {
    while (Serial.available())
    {
      int inByte = Serial.read();
      if (inByte == '!')
      {
        break;
      }
    }

    char buf[10];
    Serial.readBytesUntil('\n', buf, 9);
    Serial.flush();
  }
}

void PrintConfigData()
{
  MainSendDebug("Current configuration :");
  MainSendDebugPrintf(" - ConfigVersion : %d", config_data.ConfigVersion);
  MainSendDebugPrintf(" - Boot tentative : %d", config_data.BootFailed);
  MainSendDebugPrintf(" - Admin login : %s", config_data.adminUser);
  #ifdef DEBUG
  MainSendDebugPrintf(" - Admin psw : %s", config_data.adminPassword);
  #endif
  MainSendDebugPrintf(" - SSID : %s", config_data.ssid);
  MainSendDebugPrintf(" - Domoticz Actif : %s", (config_data.domo) ? "Y" : "N");
  MainSendDebugPrintf("   # Domoticz : %s:%u", config_data.domoticzIP, config_data.domoticzPort);
  MainSendDebugPrintf("   # DomotixzGasIdx : %u", config_data.domoticzGasIdx);
  MainSendDebugPrintf("   # DomotixzEnergyIdx : %u", config_data.domoticzEnergyIdx);
  MainSendDebugPrintf(" - MQTT Actif : %s", (config_data.mqtt) ? "Y" : "N");
  MainSendDebugPrintf("   # Send debug here : %s", (config_data.debugToMqtt) ? "Y" : "N");
  MainSendDebugPrintf("   # MQTT : mqtt://%s:***@%s:%u", config_data.mqttUser, config_data.mqttIP, config_data.mqttPort);
  MainSendDebugPrintf("   # MQTT Topic : %s", config_data.mqttTopic);
  MainSendDebugPrintf(" - interval : %u", config_data.interval);
  MainSendDebugPrintf(" - P1 In watt : %s", (config_data.watt) ? "Y" : "N");
  MainSendDebugPrintf(" - TELNET Actif : %s", (config_data.telnet) ? "Y" : "N");
  MainSendDebugPrintf("   # Send debug here : %s", (config_data.debugToTelnet) ? "Y" : "N");
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Booting...");
  MainSendDebugPrintf("Firmware: v%s", VERSION);

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(OE, OUTPUT);    // IO16 OE on the 74AHCT1G125
  digitalWrite(OE, HIGH); // Put(Keep) OE in Tristate mode
  pinMode(DR, OUTPUT);    // IO4 Data Request
  digitalWrite(DR, LOW);  // DR low (only goes high when we want to receive data)

  MainSendDebug("Load configuration from EEprom");

  EEPROM.begin(sizeof(struct settings));
  EEPROM.get(0, config_data);

  // Si la version de la configuration n'est celle attendu, on reset !
  if (config_data.ConfigVersion != SETTINGVERSION || config_data.BootFailed >= MAXBOOTFAILURE)
  {
    //Show to user is reseted !
    blink(10, 30);
    
    if (config_data.ConfigVersion != SETTINGVERSION)
    {
      MainSendDebugPrintf("Config file version is wrong (wanted:%d actual:%d)", SETTINGVERSION, config_data.ConfigVersion);
    }
    else
    {
      MainSendDebugPrintf("Too many boot fail (nbr:%d), Reset config !", config_data.BootFailed);
    }
    config_data = (settings){SETTINGVERSION, 0, true, "ssid", "password", "192.168.1.12", 8080, 1234, 1235, "sensors/power/p1meter", "10.0.0.3", 1883, "", "", 30, false, true, false, false, false, true, "adminpwd", ""};
  }
  else
  {
    config_data.BootFailed++;
  }
  
  //Save config with boot fail updated
  EEPROM.put(0, config_data);
  EEPROM.commit();
  
  blink(2, 200);
  
  PrintConfigData();

  WifiClient = new WifiMgr(config_data);
  
  if (config_data.mqtt)
  {
    MQTTClient = new MQTTMgr(WifiClient->WifiCom, config_data);
  }

  if (config_data.telnet)
  {
    TelnetServer = new TelnetMgr(config_data);
  }

  DataReaderP1 = new P1Reader(config_data);
  HTTPClient = new HTTPMgr(config_data, *TelnetServer, *MQTTClient, *DataReaderP1);
  JSONClient = new JSONMgr(config_data, *DataReaderP1);

  alignToTelegram();
  DataReaderP1->state = WAITING; // signal that we are waiting for a valid start char (aka /)
  WatchDogsTimer = millis();

  WifiClient->OnWifiEvent(EventOnWifi);
  WifiClient->Connect();
  HTTPClient->start_webservices();
}

void doWatchDogs()
{
  if (ESP.getFreeHeap() < 2000) // watchdog, in case we still have a memery leak
  {
    MainSendDebug("[WDG] FATAL : Memory leak !");
    ESP.reset();
  }

  if (millis() - DataReaderP1->LastSample > 300000)
  {
    Serial.flush();
    MainSendDebug("[WDG] No data in 300 sec, restarting monitoring");

    DataReaderP1->ResetnextUpdateTime();
  }

  if (WifiClient->AsAP() && (millis() - WifiClient->APtimer > 600000))
  {
    MainSendDebug("[WDG] No wifi, restart");
    ESP.reset(); // we have been in AP mode for 600 sec.
  }
}

void loop()
{
  WifiClient->DoMe();
  DataReaderP1->DoMe();
  HTTPClient->DoMe();

  if (TelnetServer != nullptr)
  {
    TelnetServer->DoMe();
  }

  if (MQTTClient != nullptr && !WifiClient->AsAP())
  {
    MQTTClient->doMe();
  }

  if (DataReaderP1->datagramValid && (DataReaderP1->state == DONE) && (WifiClient->WifiCom.status() == WL_CONNECTED))
  {
    if (MQTTClient != nullptr)
    {
      if (MQTTClient->MqttDelivered)
      {
        MQTTClient->MqttDelivered = false; // reset
      }
    }

    if (config_data.domo)
    {
      JSONClient->DoMe();
    }

    if (TelnetServer != nullptr)
    {
      TelnetServer->SendDataGram(DataReaderP1->datagram);
    }

    DataReaderP1->datagramValid = false; // reset
    DataReaderP1->state = WAITING;
  }

  if (millis() > WatchDogsTimer)
  {
    doWatchDogs();
    WatchDogsTimer = millis() + 22000;
  }
  
  //reset boot-failed
  if (config_data.BootFailed != 0)
  {
    config_data.BootFailed = 0;
    EEPROM.put(0, config_data);
    EEPROM.commit();
    MainSendDebug("Reset boot failed");
  }
}
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

#define FRENCH // NEDERLANDS,SWEDISH,GERMAN,FRENCH
#define DEBUG_SERIAL_P1

#define MAXBOOTFAILURE 3 //reset setting if boot fail more than this
#define WATCHDOGINTERVAL 30000;

#include <Arduino.h>
#include <EEPROM.h>
#include "GlobalVar.h"

char clientName[CLIENTNAMESIZE];
unsigned long WatchDogsTimer = millis() + WATCHDOGINTERVAL;

settings config_data;

#include "WifiMgr.h"
WifiMgr *WifiClient;

#include "MQTT.h"
MQTTMgr *MQTTClient;

#include "TelnetMgr.h"
TelnetMgr *TelnetServer;

#include "LogP1Mgr.h"
LogP1Mgr *LogP1;

#include "P1Reader.h"
P1Reader *DataReaderP1;

#include "DomoticzMgr.h"
DomoticzMgr *DomoClient;

#include "HTTPMgr.h"
HTTPMgr *HTTPClient;

ADC_MODE(ADC_VCC); // allows you to monitor the internal VCC level;

void MainSendDebug(const char *payload)
{
  #ifdef DEBUG_SERIAL_P1
  Serial.println(payload);
  #endif
  
  if (MQTTClient != nullptr)
  {
    MQTTClient->SendDebug(payload);
  }
  if (TelnetServer != nullptr)
  {
    TelnetServer->SendDebug(payload);
  }
  if (DomoClient != nullptr)
  {
    DomoClient->SendDebug(payload);
  }
}

void MainSendDebugPrintf(const char *format, ...)
{
    const int initialBufferSize = 128;
    const int maxBufferSize = 1024;
    char* buffer = nullptr;
    int bufferSize = initialBufferSize;
    int length = 0;
    va_list args;

    do
    {
        delete[] buffer;  // Safe to call on nullptr in first iteration
        buffer = new char[bufferSize];
        
        va_start(args, format);
        length = vsnprintf(buffer, bufferSize, format, args);
        va_end(args);

        if (length < 0)
        {
            delete[] buffer;
            return;
        }

        if (length >= bufferSize)
        {
            bufferSize *= 2;  // Double the buffer size
            if (bufferSize > maxBufferSize)
            {
                delete[] buffer;
                return;
            }
        }
    } while (length >= bufferSize);

    MainSendDebug(buffer);
    delete[] buffer;
}

/// @brief Non-blocking delay using yield() to yield control back to the CPU.
/// @param ms Time delay in ms
void Yield_Delay(unsigned long ms)
{
  unsigned long WaitUnitl = millis() + ms;

  while(millis() <= WaitUnitl)
  {
    yield();
  }
}


/// @brief Permet de faire clignoter LED_BUILTIN via un toggle
/// @param t Nombre de clignotement
/// @param speed Vitesse de clignotement (onde carrée)
void blink(int t, unsigned long speed)
{
  t = t * 2;
  for (int i = 0; i <= t; i++)
  {
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    delay(speed);
  }
}

#ifdef DEBUG_SERIAL_P1
void PrintConfigData()
{
  MainSendDebug("[Core] Setting :");
  MainSendDebugPrintf(" - ConfigVersion : %d", config_data.ConfigVersion);
  MainSendDebugPrintf(" - Boot tentative : %d", config_data.BootFailed);
  MainSendDebugPrintf(" - Admin login : %s", config_data.adminUser);
  //MainSendDebugPrintf(" - Admin psw : %s", config_data.adminPassword);
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
  MainSendDebugPrintf(" - Invert high/low tarif: %s", (config_data.InverseHigh_1_2_Tarif) ? "Y" : "N");
  MainSendDebugPrintf(" - TELNET Actif : %s", (config_data.telnet) ? "Y" : "N");
  MainSendDebugPrintf("   # Send debug here : %s", (config_data.debugToTelnet) ? "Y" : "N");
  Yield_Delay(20);
}
#endif

/// @brief Génération du nom unique se reposant sur HOSTNAME et les 4 octets finaux de l'adresse MAC
void SetName()
{
    const u_int8_t macLen = 17;
    char macAddr[18]; // 18 caractères pour inclure l'octet nul

    // Obtenir l'adresse MAC et la copier dans le tableau
    strncpy(macAddr, WiFi.macAddress().c_str(), sizeof(macAddr) - 1);
    strcpy(clientName, HOSTNAME);
    strcat(clientName, "-");
    
    // Pointer vers le début des 2 derniers groupes (position -5:-2 et -2:fin)
    const char* macStr = macAddr;
    
    // Copie les 2 derniers octets sans les ':'
    char lastBytes[5];
    lastBytes[0] = macStr[macLen-5];
    lastBytes[1] = macStr[macLen-4];
    lastBytes[2] = macStr[macLen-2];
    lastBytes[3] = macStr[macLen-1];
    lastBytes[4] = '\0';
    
    strcat(clientName, lastBytes);
}

/// @brief Obtenir le nom unique se reposant sur HOSTNAME et les 4 octets finaux de l'adresse MAC
/// @return 
char* GetClientName()
{
  return clientName;
}

void setup()
{
  #ifdef DEBUG_SERIAL_P1
  Serial.begin(SERIALSPEED);
  Serial.println("Booting...");
  #endif
  SetName();
  MainSendDebugPrintf("[Core] Firmware: v%s.%u", VERSION, BUILD_DATE);

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(OE, OUTPUT);    // IO16 OE on the 74AHCT1G125
  digitalWrite(OE, HIGH); // Put(Keep) OE in Tristate mode
  pinMode(DR, OUTPUT);    // IO4 Data Request
  digitalWrite(DR, LOW);  // DR low (only goes high when we want to receive data)

  MainSendDebug("[Core] Load configuration from EEprom");

  EEPROM.begin(sizeof(struct settings));
  EEPROM.get(0, config_data);

  // Si la version de la configuration n'est celle attendu, on reset !
  if (config_data.ConfigVersion != SETTINGVERSION || config_data.BootFailed >= MAXBOOTFAILURE)
  {    
    if (config_data.ConfigVersion != SETTINGVERSION)
    {
      MainSendDebugPrintf("[Core] Reset settings (wanted:%d actual:%d)", SETTINGVERSION, config_data.ConfigVersion);
    }
    else
    {
      MainSendDebugPrintf("[Core] Too many boot fail (nbr:%d), Reset config !", config_data.BootFailed);
    }

    //Show to user is reseted !
    blink(20, 50UL);

    config_data = (settings){SETTINGVERSION, 0, true, "", "", "10.0.0.3", 8084, 0, 0, "dsmr", "10.0.0.3", 1883, "", "", 60, false, false, false, false, false, false, "", "", false, false, 0};
  }
  else
  {
    config_data.BootFailed++;
  }
  
  //Save config with boot fail updated
  EEPROM.put(0, config_data);
  EEPROM.commit();
  
  #ifdef DEBUG_SERIAL_P1
  PrintConfigData();
  #endif

  WifiClient = new WifiMgr(config_data);
  DataReaderP1 = new P1Reader(config_data);

  if (config_data.telnet)
  {
    TelnetServer = new TelnetMgr(config_data, *DataReaderP1);
  }

  if (config_data.mqtt)
  {
    MQTTClient = new MQTTMgr(config_data, *WifiClient, *DataReaderP1);
  }

  if (config_data.domo)
  {
    DomoClient = new DomoticzMgr(config_data, *DataReaderP1);
  }
  
  LogP1 = new LogP1Mgr(config_data, *DataReaderP1);
  HTTPClient = new HTTPMgr(config_data, *TelnetServer, *MQTTClient, *DataReaderP1, *LogP1);

  blink(2, 500UL); // signale que le module est prêt !

  WifiClient->Connect();
  HTTPClient->start_webservices();
}

/// @brief Check la quantité de RAM disponible et reset si besoins le nombre d'erreur de boot
void doWatchDogs()
{
  if (ESP.getFreeHeap() < 2000) // watchdog, in case we still have a memery leak
  {
    MainSendDebug("[Core] FATAL : Memory leak !");
    ESP.reset();
  }
  
  //reset boot-failed
  if (config_data.BootFailed != 0)
  {
    config_data.BootFailed = 0;
    EEPROM.put(0, config_data);
    EEPROM.commit();
  }

  //reset Watchdog
  WatchDogsTimer = millis() + WATCHDOGINTERVAL;
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

  if (millis() > WatchDogsTimer)
  {
    doWatchDogs();
  }
}

/// @brief Permet de demandé le redémarrage de l'ESP en prévenant les modules
/// @param delay Durée avant le redémarrage en ms
void RequestRestart(unsigned long delay)
{
  MainSendDebug("[Core] Reboot requested !!!");

  if (TelnetServer != nullptr)
  {
    TelnetServer->stop();
  }
  
  if (MQTTClient != nullptr )
  {
    MQTTClient->stop();
  }

  Yield_Delay(delay);
  ESP.restart();
}
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
//#define DEBUG_SERIAL_P1

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
            MainSendDebug("Error: Invalid format string in MainSendDebugPrintf");
            delete[] buffer;
            return;
        }

        if (length >= bufferSize)
        {
            bufferSize *= 2;  // Double the buffer size
            if (bufferSize > maxBufferSize)
            {
                MainSendDebug("Error: Debug message too long in MainSendDebugPrintf");
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

void blink(int t, unsigned long speed)
{
  for (int i = 0; i <= t; i++)
  {
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    delay(speed);
  }
  digitalWrite(LED_BUILTIN, HIGH);
}

void PrintConfigData()
{
  MainSendDebug("Current configuration :");
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
  MainSendDebugPrintf(" - P1 In watt : %s", (config_data.watt) ? "Y" : "N");
  MainSendDebugPrintf(" - TELNET Actif : %s", (config_data.telnet) ? "Y" : "N");
  MainSendDebugPrintf("   # Send debug here : %s", (config_data.debugToTelnet) ? "Y" : "N");
  Yield_Delay(20);
}

void setup()
{
  #ifdef DEBUG_SERIAL_P1
  Serial.begin(SERIALSPEED);
  Serial.println("Booting...");
  #endif
  MainSendDebugPrintf("Firmware: v%s.%u", VERSION, BUILD_DATE);

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
    if (config_data.ConfigVersion != SETTINGVERSION)
    {
      MainSendDebugPrintf("Config file version is wrong (wanted:%d actual:%d)", SETTINGVERSION, config_data.ConfigVersion);
    }
    else
    {
      MainSendDebugPrintf("Too many boot fail (nbr:%d), Reset config !", config_data.BootFailed);
    }

    //Show to user is reseted !
    blink(20, 50UL);

    config_data = (settings){SETTINGVERSION, 0, true, "", "", "192.168.1.12", 8080, 1234, 1235, "sensors/power/p1meter", "10.0.0.3", 1883, "", "", 60, false, true, false, false, false, true, "", ""};
  }
  else
  {
    config_data.BootFailed++;
  }
  
  //Save config with boot fail updated
  EEPROM.put(0, config_data);
  EEPROM.commit();
  
  blink(2, 200UL);
  
  PrintConfigData();

  WifiClient = new WifiMgr(config_data);
  
  if (config_data.telnet)
  {
    TelnetServer = new TelnetMgr(config_data,*DataReaderP1);
  }

  DataReaderP1 = new P1Reader(config_data);

  if (config_data.mqtt)
  {
    MQTTClient = new MQTTMgr(config_data, *WifiClient, *DataReaderP1);
  }

  HTTPClient = new HTTPMgr(config_data, *TelnetServer, *MQTTClient, *DataReaderP1);
  JSONClient = new JSONMgr(config_data, *DataReaderP1);

  WatchDogsTimer = millis();

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

  /*if (millis() - DataReaderP1->LastSample > 300000)
  {
    MainSendDebug("[WDG] No data in 300 sec, restarting monitoring");
    
    DataReaderP1->ResetnextUpdateTime();
  }

  if (WifiClient->AsAP() && (millis() - WifiClient->APtimer > 600000))
  {
    MainSendDebug("[WDG] No wifi, restart");
    ESP.reset(); // we have been in AP mode for 600 sec.
  }*/
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

  if (MQTTClient != nullptr && WifiClient->IsConnected())
  {
    MQTTClient->doMe();
  }

  if (DataReaderP1->datagramValid && (DataReaderP1->state == DONE) && WifiClient->IsConnected())
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
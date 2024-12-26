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
 * MQTT part based on https://github.com/daniel-jong/esp8266_p1meter/blob/master/esp8266_p1meter/esp8266_p1meter.ino
 */

#include <MQTT.h>

MQTTMgr::MQTTMgr(settings &currentConf, WifiMgr &currentLink, P1Reader &currentP1) : conf(currentConf), WifiClient(currentLink), DataReaderP1(currentP1)
{
  mqtt_connect();

  WifiClient.OnWifiEvent([this](bool b, wl_status_t s1, wl_status_t s2)
  {
    if (b)
    {
      nextMQTTreconnectAttempt = 0;
      mqtt_connect();
    }
  });

  DataReaderP1.OnNewDatagram([this]()
  {
    MQTT_reporter();
  });

  // Configuration des callbacks MQTT
  mqtt_client.onConnect([this](bool sessionPresent)
  {
    onMqttConnect(sessionPresent);
  });
  
  mqtt_client.onDisconnect([this](AsyncMqttClientDisconnectReason reason)
  {
    onMqttDisconnect(reason);
  });
}

void MQTTMgr::onMqttDisconnect(AsyncMqttClientDisconnectReason reason)
{
  _state = DISCONNECTED;
  CountError++;
  MainSendDebugPrintf("[MQTT] Disconnected (%u)", reason);

  if (CountError >= MAXERROR)
  {
    MainSendDebug("[MQTT] Max error to connect, reset MQTT client");
    mqtt_client.disconnect(true);
    mqtt_client.clearQueue();
    CountError = 0;
  }

  nextMQTTreconnectAttempt = millis() + RETRYTIME; //try reconnect
}

void MQTTMgr::onMqttConnect(bool sessionPresent)
{
  _state = CONNECTED;
  CountError = 0;
  MainSendDebug("[MQTT] connected");

  // Once connected, publish an announcement...
  send_char("State/status", "running");
  send_char("State/Version", VERSION);
  send_char("State/IP", WifiClient.CurrentIP().c_str());
}

bool MQTTMgr::IsConnected()
{
  return mqtt_client.connected();
}

void MQTTMgr::stop()
{
  send_char("State/status", "stopping");
}

bool MQTTMgr::mqtt_connect()
{
  if (mqtt_client.connected())
  {
    return true;
  }

  if (_state != CONNECTING && millis() > nextMQTTreconnectAttempt)
  {
    MainSendDebugPrintf("[MQTT] connect to %s:%u ...", conf.mqttIP, conf.mqttPort);

    //Setting connection
    if (strlen(conf.mqttUser) != 0 && strlen(conf.mqttPass) != 0)
    {
      mqtt_client.setCredentials(conf.mqttUser, conf.mqttPass);
    }
    mqtt_client.setServer(conf.mqttIP, conf.mqttPort);
    mqtt_client.setClientId(GetClientName());
   
    // Attempt to connect
    _state = CONNECTING;
    mqtt_client.connect();
  }
  
  //Reply with the new status
  return mqtt_client.connected();
}


void MQTTMgr::send_float(String name, float metric) // added *long
{
  char value[20];
  dtostrf(metric, 3, 3, value);

  String mtopic = String(conf.mqttTopic) + "/" + name;
  send_msg(mtopic.c_str(), value); // output
}

void MQTTMgr::send_char(String name, const char *metric)
{
  String mtopic = String(conf.mqttTopic) + "/" + name;
  send_msg(mtopic.c_str(), metric);
}

void MQTTMgr::send_uint32_t(String name, uint32_t metric)
{
  char value_buffer[11];  // uint32_t max = 4294967295 (10 chiffres + \0)
  uint32ToChar(metric, value_buffer);

  String mtopic = String(conf.mqttTopic) + "/" + name;
  send_msg(mtopic.c_str(), value_buffer);
}

/// @brief Send a message to a broker topic
/// @param topic 
/// @param payload 
void MQTTMgr::send_msg(const char *topic, const char *payload)
{
    if (!mqtt_client.connected())
    {
      mqtt_connect();
    }

    if (payload[0] == 0)
    {
      return; //nothing to report
    }
    mqtt_client.publish(topic, 2, true, payload);
}

char* MQTTMgr::uint32ToChar(uint32_t value, char* buffer)
{
    char* p = buffer;
    
    if (value == 0)
    {
        *p++ = '0';
        *p = '\0';
        return buffer;
    }
    
    // On divise par 10 et on convertit de droite à gauche
    char* start = p;
    while (value)
    {
        *p++ = '0' + (value % 10);
        value /= 10;
    }
    *p = '\0';
    
    // On inverse la chaîne
    char* end = p - 1;
    while (start < end)
    {
        char temp = *start;
        *start = *end;
        *end = temp;
        start++;
        end--;
    }
    
    return buffer;
}

void MQTTMgr::SendDebug(String payload)
{
  if (conf.debugToMqtt && mqtt_client.connected())
  {
    char charArray[payload.length() + 1]; // +1 pour le caractère nul
    payload.toCharArray(charArray, sizeof(charArray));
    charArray[sizeof(charArray) - 1] = '\0';
    send_char("State/Logging", charArray);
  }
}

void MQTTMgr::MQTT_reporter()
{
  if (!DataReaderP1.dataEnd)
  {
    //Pas de donnée valide a envoyer
    return;
  }

  MainSendDebug("[MQTT] Send P1 data");

  //no DSMR valid :
  send_char("equipmentName", DataReaderP1.meterName.c_str());

  send_char("equipmentID", DataReaderP1.DataReaded.equipmentId);
  send_char("reading/timestamp", DataReaderP1.DataReaded.P1timestamp);

  send_float("reading/electricity_delivered_1", DataReaderP1.DataReaded.electricityUsedTariff1);
  send_float("reading/electricity_delivered_2", DataReaderP1.DataReaded.electricityUsedTariff2);
  send_float("reading/electricity_returned_1", DataReaderP1.DataReaded.electricityReturnedTariff1);
  send_float("reading/electricity_returned_2", DataReaderP1.DataReaded.electricityReturnedTariff2);
  send_float("reading/electricity_currently_delivered", DataReaderP1.DataReaded.actualElectricityPowerDeli);
  send_float("reading/electricity_currently_returned", DataReaderP1.DataReaded.actualElectricityPowerRet);

  send_float("reading/phase_currently_delivered_l1", DataReaderP1.DataReaded.activePowerL1P);
  send_float("reading/phase_currently_delivered_l2", DataReaderP1.DataReaded.activePowerL2P);
  send_float("reading/phase_currently_delivered_l3", DataReaderP1.DataReaded.activePowerL3P);
  send_float("reading/phase_currently_returned_l1", DataReaderP1.DataReaded.activePowerL1NP);
  send_float("reading/phase_currently_returned_l2", DataReaderP1.DataReaded.activePowerL2NP);
  send_float("reading/phase_currently_returned_l3", DataReaderP1.DataReaded.activePowerL3NP);
  send_float("reading/phase_voltage_l1", DataReaderP1.DataReaded.instantaneousVoltageL1);
  send_float("reading/phase_voltage_l2", DataReaderP1.DataReaded.instantaneousVoltageL2);
  send_float("reading/phase_voltage_l3", DataReaderP1.DataReaded.instantaneousVoltageL3);

  send_char("consumption/gas/delivered", DataReaderP1.DataReaded.gasReceived5min);
  
  send_char("meter-stats/dsmr_version", DataReaderP1.DataReaded.P1version);
  send_uint32_t("meter-stats/electricity_tariff", DataReaderP1.DataReaded.tariffIndicatorElectricity);
  send_uint32_t("meter-stats/power_failure_count", DataReaderP1.DataReaded.numberLongPowerFailuresAny);
  send_uint32_t("meter-stats/long_power_failure_count", DataReaderP1.DataReaded.numberLongPowerFailuresAny);
  send_uint32_t("meter-stats/short_power_drops", DataReaderP1.DataReaded.numberVoltageSagsL1);
  send_uint32_t("meter-stats/short_power_peaks", DataReaderP1.DataReaded.numberVoltageSwellsL1);

  LastReportinMillis = millis();

  return;
}
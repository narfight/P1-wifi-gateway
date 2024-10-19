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
 * MQTT part based on https://github.com/daniel-jong/esp8266_p1meter/blob/master/esp8266_p1meter/esp8266_p1meter.ino
 */

#include <MQTT.h>

MQTTMgr::MQTTMgr(settings &currentConf, WifiMgr &currentLink, P1Reader &currentP1) : conf(currentConf), WifiClient(currentLink), DataReaderP1(currentP1)
{
  WifiClient.OnWifiEvent([this](bool b, wl_status_t s1, wl_status_t s2)
  {
      if (b)
      {
        nextMQTTreconnectAttempt = 0;
      }
  });
  
  mqtt_client.setClient(WifiClient.WifiCom);
  mqtt_client.setServer(conf.mqttIP, conf.mqttPort);
}

bool MQTTMgr::IsConnected()
{
  return mqtt_client.connected();
}

void MQTTMgr::doMe()
{
  if (mqtt_connect())
  {
    mqtt_client.loop();
    if (DataReaderP1.datagramValid && nextMQTTReport < millis())
    {
      nextMQTTReport = millis() + conf.interval * 1000;
      MQTT_reporter();
    }
  }
}

bool MQTTMgr::mqtt_connect()
{
  if (!mqtt_client.connected())
  {
    if (millis() > nextMQTTreconnectAttempt)
    {
      MainSendDebugPrintf("[MQTT] Connect to %s:%u", conf.mqttIP, conf.mqttPort);
      
      // Attempt to connect
      if (mqtt_client.connect(HOSTNAME, conf.mqttUser, conf.mqttPass))
      {
        MainSendDebug("[MQTT] connected");

        // Once connected, publish an announcement...
        mqtt_send_metric("State/Payload", "p1 gateway running");
        mqtt_send_metric("State/Version", VERSION);
        mqtt_send_metric("State/IP", WifiClient.CurrentIP().c_str());
      }
      else
      {
        MainSendDebugPrintf("[MQTT] Connection failed : rc=%d", mqtt_client.state());
        nextMQTTreconnectAttempt = millis() + 2000; // try again in 2 seconds
      }
    }
  }
  
  //Reply with the new status
  return mqtt_client.connected();
}

/// @brief Send a message to a broker topic
/// @param topic 
/// @param payload 
void MQTTMgr::send_msg(const char *topic, const char *payload)
{
    if (payload[0] == 0)
    {
      return; //nothing to report
    }

    if (!mqtt_client.publish(topic, payload, false))
    {
        MainSendDebugPrintf("[MQTT] Error to send to topic : %s", topic);
    }
}

void MQTTMgr::send_metric(String name, float metric) // added *long
{
  char value[20];
  dtostrf(metric, 3, 3, value);

  String mtopic = String(conf.mqttTopic) + "/" + name;
  send_msg(mtopic.c_str(), value); // output
}

void MQTTMgr::mqtt_send_metric(String name, const char *metric)
{
  String mtopic = String(conf.mqttTopic) + "/" + name;
  send_msg(mtopic.c_str(), metric);
}

void MQTTMgr::SendDebug(String payload)
{
  if (conf.debugToMqtt && mqtt_client.connected())
  {
    char charArray[payload.length() + 1]; // +1 pour le caractère nul
    payload.toCharArray(charArray, sizeof(charArray));
    charArray[sizeof(charArray) - 1] = '\0';
    mqtt_send_metric("State/Logging", charArray);
  }
}

void MQTTMgr::MQTT_reporter()
{
  if (!DataReaderP1.datagramValid)
  {
    //Pas de donnée valide a envoyer
    MqttDelivered = false;
    return;
  }

  MainSendDebug("[MQTT] Send data");

  mqtt_send_metric("equipmentID", DataReaderP1.DataReaded.equipmentId);

  mqtt_send_metric("reading/electricity_delivered_1", DataReaderP1.DataReaded.electricityUsedTariff1);
  mqtt_send_metric("reading/electricity_delivered_2", DataReaderP1.DataReaded.electricityUsedTariff2);
  mqtt_send_metric("reading/electricity_returned_1", DataReaderP1.DataReaded.electricityReturnedTariff1);
  mqtt_send_metric("reading/electricity_returned_2", DataReaderP1.DataReaded.electricityReturnedTariff2);
  mqtt_send_metric("reading/electricity_currently_delivered", DataReaderP1.DataReaded.actualElectricityPowerDeli);
  mqtt_send_metric("reading/electricity_currently_returned", DataReaderP1.DataReaded.actualElectricityPowerRet);

  mqtt_send_metric("reading/phase_currently_delivered_l1", DataReaderP1.DataReaded.activePowerL1P);
  mqtt_send_metric("reading/phase_currently_delivered_l2", DataReaderP1.DataReaded.activePowerL2P);
  mqtt_send_metric("reading/phase_currently_delivered_l3", DataReaderP1.DataReaded.activePowerL3P);
  mqtt_send_metric("reading/phase_currently_returned_l1", DataReaderP1.DataReaded.activePowerL1NP);
  mqtt_send_metric("reading/phase_currently_returned_l2", DataReaderP1.DataReaded.activePowerL2NP);
  mqtt_send_metric("reading/phase_currently_returned_l3", DataReaderP1.DataReaded.activePowerL3NP);
  mqtt_send_metric("reading/phase_voltage_l1", DataReaderP1.DataReaded.instantaneousVoltageL1);
  mqtt_send_metric("reading/phase_voltage_l2", DataReaderP1.DataReaded.instantaneousVoltageL2);
  mqtt_send_metric("reading/phase_voltage_l3", DataReaderP1.DataReaded.instantaneousVoltageL3);

  mqtt_send_metric("consumption/gas/delivered", DataReaderP1.DataReaded.gasReceived5min);

  send_metric("meter-stats/actual_tarif_group", DataReaderP1.DataReaded.tariffIndicatorElectricity[3]);
  mqtt_send_metric("meter-stats/power_failure_count", DataReaderP1.DataReaded.numberLongPowerFailuresAny);
  mqtt_send_metric("meter-stats/long_power_failure_count", DataReaderP1.DataReaded.numberLongPowerFailuresAny);
  mqtt_send_metric("meter-stats/short_power_drops", DataReaderP1.DataReaded.numberVoltageSagsL1);
  mqtt_send_metric("meter-stats/short_power_peaks", DataReaderP1.DataReaded.numberVoltageSwellsL1);

  MqttDelivered = true;
  LastReportinMillis = millis();

  return;
}
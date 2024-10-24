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

#ifndef TelnetMgr_H
#define TelnetMgr_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <map>
#include "Debug.h"
#include "GlobalVar.h"
#include "P1Reader.h"

#define MAX_SRV_CLIENTS 5
#define STACK_PROTECTOR 1512  // bytes
#define TELNETPORT 23
#define TELNET_REPPORT_INTERVAL_SEC 10

class TelnetMgr
{
  private:
  settings& conf;
  P1Reader &P1Captor;
  std::map<int, bool> authenticatedClients;
  std::map<int, unsigned long> lastActivityTime;
  void handleNewConnections();
  int findFreeClientSlot();
  void checkInactiveClients();
  bool authenticateClient(WiFiClient &client, int clientId);
  bool readWithTimeout(WiFiClient &client, const char* prompt, unsigned long timeout);
  WiFiServer telnet;
  WiFiClient telnetClients[MAX_SRV_CLIENTS];
  void handleClientActivity();
  void processCommand(int clientId, const String &command);
  void commandeHelp(int clientId);
  void closeConnection(int clientId);
  unsigned long NextReportTime = millis();
  public:
  explicit TelnetMgr(settings& currentConf, P1Reader &currentP1);
  void DoMe();
  void stop();
  void SendDataGram(String Diagram);
  void SendDebug(String payload);
};
#endif
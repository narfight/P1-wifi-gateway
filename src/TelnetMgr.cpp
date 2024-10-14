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

#include "TelnetMgr.h"

TelnetMgr::TelnetMgr(settings &currentConf) : conf(currentConf), telnet(TELNETPORT)
{
    MainSendDebugPrintf("[TELNET] Starting");
    Yield_Delay(100);
    telnet.setNoDelay(true);
    telnet.begin();
}

bool TelnetMgr::authenticateClient(WiFiClient &client, int clientId)
{
    const unsigned long AUTH_TIMEOUT = 30000; // 30 secondes
    const int MAX_ATTEMPTS = 3;

    for (int attempt = 0; attempt < MAX_ATTEMPTS; ++attempt)
    {
        if (!readWithTimeout(client, "Login: ", AUTH_TIMEOUT)) return false;
        String username = client.readStringUntil('\n');
        username.trim();

        if (!readWithTimeout(client, "Password: ", AUTH_TIMEOUT)) return false;
        String password = client.readStringUntil('\n');
        password.trim();

        if (username == conf.adminUser && password == conf.adminPassword) {
            client.println("Authentication successful.");
            authenticatedClients[clientId] = true;
            lastActivityTime[clientId] = millis();
            return true;
        }

        client.printf("Authentication failed. %d attempts remaining.\n", MAX_ATTEMPTS - attempt - 1);
        Yield_Delay(1000 * (attempt + 1)); // Délai croissant entre les tentatives
    }

    client.println("Max attempts reached. Connection closed.");
    return false;
}

bool TelnetMgr::readWithTimeout(WiFiClient &client, const char* prompt, unsigned long timeout)
{
    client.print(prompt);
    unsigned long start = millis();
    while (!client.available() && millis() - start < timeout)
    {
        Yield_Delay(10);
    }

    if (millis() - start >= timeout)
    {
        client.println("Timeout. Connection closed.");
        return false;
    }
    return true;
}

void TelnetMgr::handleClientActivity()
{
    for (int i = 0; i < MAX_SRV_CLIENTS; i++)
    {
        if (telnetClients[i] && telnetClients[i].available())
        {
            String command = telnetClients[i].readStringUntil('\n');
            command.trim();
            processCommand(i, command);
            lastActivityTime[i] = millis();
        }
    }
}

void TelnetMgr::closeConnection(int clientId)
{
    telnetClients[clientId].stop();
    authenticatedClients.erase(clientId);
    lastActivityTime.erase(clientId);
    MainSendDebugPrintf("[TELNET] Closed connection for client %d", clientId);
}

void TelnetMgr::processCommand(int clientId, const String &command)
{
    if (command == "exit")
    {
        telnetClients[clientId].println("Goodbye!");
        closeConnection(clientId);
    }
    else if (command == "reboot")
    {
        MainSendDebugPrintf("[TELNET] User request reboot !");
        Yield_Delay(1000);
        ESP.restart();
    }
    else if (command == "help") 
    {
        telnetClients[clientId].println("Available commands: exit, reboot, help");
    }
    else
    {
        telnetClients[clientId].println("Unknown command. Type 'help' for available commands.");
    }
}

bool TelnetMgr::isClientAuthenticated(int clientId)
{
    return authenticatedClients.find(clientId) != authenticatedClients.end() && authenticatedClients[clientId];
}

void TelnetMgr::DoMe()
{
    handleClientActivity();
    checkInactiveClients();
    handleNewConnections();
}

void TelnetMgr::handleNewConnections()
{
    if (telnet.hasClient())
    {
        int i = findFreeClientSlot();
        if (i < MAX_SRV_CLIENTS)
        {
            telnetClients[i] = telnet.available();
            if (authenticateClient(telnetClients[i], i))
            {
                telnetClients[i].printf("Welcome! Your session ID is %d.\n", i);
                MainSendDebugPrintf("[TELNET] New authenticated session (Id:%d)", i);
            }
            else
            {
                closeConnection(i);
            }
        }
        else
        {
            telnet.available().println("Server is busy. Try again later.");
            MainSendDebugPrintf("[TELNET] Server is busy with %d active connections", MAX_SRV_CLIENTS);
        }
    }
}

int TelnetMgr::findFreeClientSlot()
{
    for (int i = 0; i < MAX_SRV_CLIENTS; i++)
    {
        if (!telnetClients[i] || !telnetClients[i].connected())
        {
            return i;
        }
    }
    return MAX_SRV_CLIENTS;
}

void TelnetMgr::checkInactiveClients()
{
    const unsigned long INACTIVITY_TIMEOUT = 5 * 60 * 1000; // 5 minutes
    unsigned long currentTime = millis();

    for (int i = 0; i < MAX_SRV_CLIENTS; i++)
    {
        if (telnetClients[i] && (currentTime - lastActivityTime[i] > INACTIVITY_TIMEOUT))
        {
            telnetClients[i].println("Session timeout due to inactivity.");
            closeConnection(i);
        }
    }
}

void TelnetMgr::SendDataGram(String Diagram)
{
    TelnetReporter(Diagram);
}

void TelnetMgr::SendDebug(String payload)
{
    if (!conf.debugToTelnet)
    {
        return;
    }

    char result[100];
    snprintf(result, sizeof(result), "%s %s", "[DEBUG]", payload.c_str());

    for (int i = 0; i < MAX_SRV_CLIENTS; i++)
    {
        if (telnetClients[i])
        {
            if (telnetClients[i].availableForWrite() > 0)
            {
                telnetClients[i].println(result);
            }
        }
    }
}

void TelnetMgr::TelnetReporter(String Diagram)
{
    if (millis() < NextReportTime)
    {
        return;
    }

    NextReportTime = millis() + (TELNET_REPPORT_INTERVAL_SEC * 1000);

    int maxToTcp = 0;
    for (int i = 0; i < MAX_SRV_CLIENTS; i++)
    {
        if (telnetClients[i])
        {
            int afw = telnetClients[i].availableForWrite();
            if (afw)
            {
                if (!maxToTcp)
                {
                    maxToTcp = afw;
                }
                else
                {
                    maxToTcp = std::min(maxToTcp, afw);
                }
            }
            else
            {
                // warn but ignore congested clients
                MainSendDebugPrintf("[TELNET] Client %d is congested, kill connection.", i);
                telnetClients[i].flush();
                telnetClients[i].stop();
            }
        }
    }

    int len = Diagram.length();

    for (int i = 0; i < MAX_SRV_CLIENTS; i++)
    {
        if (telnetClients[i].availableForWrite() >= 1)
        {
            telnetClients[i].write(Diagram.c_str(), len);
        }
        else
        {
            MainSendDebugPrintf("[TELNET] Client %d is not available for writing.", i);
        }
    }  
    yield();
}
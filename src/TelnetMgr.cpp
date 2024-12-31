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

#include "TelnetMgr.h"

TelnetMgr::TelnetMgr(settings &currentConf, P1Reader &currentP1) : conf(currentConf), P1Captor(currentP1), telnet(TELNETPORT)
{
    MainSendDebugPrintf("[TELNET] Starting");
    Yield_Delay(100);
    telnet.setNoDelay(true);
    telnet.begin();

    if (conf.Repport2Telnet)
    {
        P1Captor.OnNewDatagram([this]()
        {
            SendDataGram();
        });
    }
}

bool TelnetMgr::authenticateClient(WiFiClient &client, int clientId)
{
    Yield_Delay(200); //On attend pour bien flusher tout avant !
    while (client.available()) // flush tout se qui est dans le cache
    {
        client.read();
    }

    lastActivityTime[clientId] = millis();
    
    if (strlen(conf.adminPassword) == 0)
    {
        //No password defined, no need request auth to client
        return true;
    }

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
            return true;
        }

        client.printf("Try again. %d attempts remaining.\n", MAX_ATTEMPTS - attempt - 1);
        Yield_Delay(1000 * (attempt + 1)); // Délai croissant entre les tentatives
    }

    client.println("Max attempts reached.");
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
            
            if (command.length() != 0)
            {
                processCommand(i, command);
            }
            else
            {
                telnetClients[i].printf("%s>", GetClientName());
            }
            
            lastActivityTime[i] = millis();
        }
    }
}

void TelnetMgr::closeConnection(int clientId)
{
    telnetClients[clientId].stop();
    authenticatedClients.erase(clientId);
    lastActivityTime.erase(clientId);
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
        RequestRestart(1000);
    }
    else if (command == "help") 
    {
        commandeHelp(clientId);
    }
    else if (command == "raw") 
    {
        telnetClients[clientId].println(P1Captor.datagram);
    }
    else if (command == "read") 
    {
        P1Captor.ResetnextUpdateTime();
        telnetClients[clientId].println("Done");
    }
    else
    {
        telnetClients[clientId].print("Unknown command : ");
        telnetClients[clientId].println(command);
        commandeHelp(clientId);
    }
    telnetClients[clientId].printf("%s>", GetClientName());
}
void TelnetMgr::commandeHelp(int clientId)
{
    telnetClients[clientId].println("Available commands: exit, raw, read, reboot, help");
}

void TelnetMgr::DoMe()
{
    handleClientActivity();
    checkInactiveClients();
    handleNewConnections();
}

void TelnetMgr::stop()
{
    for (int i = 0; i < MAX_SRV_CLIENTS; i++)
    {
        if (telnetClients[i] && telnetClients[i].connected())
        {
            telnetClients[i].println("Reboot, session killed.");
            closeConnection(i);
        }
    }
}

void TelnetMgr::handleNewConnections()
{
    if (telnet.hasClient())
    {
        int i = findFreeClientSlot();
        if (i < MAX_SRV_CLIENTS)
        {
            telnetClients[i] = telnet.accept();
            if (authenticateClient(telnetClients[i], i))
            {
                telnetClients[i].print("Welcome!");
                MainSendDebug("[TELNET] New session");
                telnetClients[i].printf("%s>", GetClientName());
            }
            else
            {
                closeConnection(i);
            }
        }
        else
        {
            telnet.accept().println("Server is busy. Try again later.");
            MainSendDebugPrintf("[TELNET] no slot free for new connection");
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

void TelnetMgr::SendDataGram()
{
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

    int len = P1Captor.datagram.length();

    for (int i = 0; i < MAX_SRV_CLIENTS; i++)
    {
        if (telnetClients[i].availableForWrite() >= 1)
        {
            telnetClients[i].write(P1Captor.datagram.c_str(), len);
        }
    }  
    yield();
}

void TelnetMgr::SendDebug(String payload)
{
    if (!conf.debugToTelnet)
    {
        return;
    }

    // Calculer la taille nécessaire pour le buffer
    // +3 pour : 2 espaces et le caractère nul '\0'
    size_t totalLength = strlen("[DEBUG]") + payload.length() + 3;
    
    char* result = new char[totalLength];
    
    // Formater la chaîne
    snprintf(result, totalLength, "%s %s", "[DEBUG]", payload.c_str());

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
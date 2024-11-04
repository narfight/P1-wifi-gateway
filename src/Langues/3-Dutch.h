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
 
 * Additionally, please note that the original source code of this file
 * may contain portions of code derived from (or inspired by)
 * previous works by:
 *
 * Ronald Leenes (https://github.com/romix123/P1-wifi-gateway and http://esp8266thingies.nl)
 * MQTT part based on https://github.com/daniel-jong/esp8266_p1meter/blob/master/esp8266_p1meter/esp8266_p1meter.ino
 */

#if LANGUAGE == 3
#define LANG_LOADED //controleer bij compilatie of de taal correct is ingesteld
#define LANG_HEADERLG "nl"
#define LANG_H1Welcome "Stel een wachtwoord in"
#define LANG_PSWD1 "Wachtwoord"
#define LANG_PSWD2 "Herhaal wachtwoord"
#define LANG_PSWDERROR "De wachtwoorden komen niet overeen."
#define LANG_MENU "Menu"
#define LANG_MENUP1 "Gemeten waarden"
#define LANG_MENUGraph24 "Grafiek over 24 uur"
#define LANG_MENUConf "Configuratie"
#define LANG_MENUOTA "Firmware bijwerken"
#define LANG_MENURESET "Fabrieksinstellingen herstellen"
#define LANG_MENUREBOOT "Herstarten"
#define LANG_MENUPASSWORD "Inloggegevens"
#define LANG_H1DATA "Gegevens"
#define LANG_ConfH1 "WiFi- en moduleconfiguratie"
#define LANG_ACTIONSAVE "Opslaan"
#define LANG_Conf_Saved "Instellingen zijn succesvol opgeslagen."
#define LANG_ConfReboot "De module zal nu opnieuw starten. Dit duurt ongeveer een minuut."
#define LANG_ConfLedStart "De blauwe LED zal 2 keer knipperen wanneer de module is opgestart."
#define LANG_ConfLedError "Als de blauwe LED blijft branden, is de configuratie mislukt. Verbind opnieuw met het WiFi-netwerk <b>%s</b> om de instellingen aan te passen."
#define LANG_ConfP1H2 "Meteropties"
#define LANG_ConfWIFIH2 "WiFi-instellingen"
#define LANG_ConfSSID "SSID"
#define LANG_ConfWIFIPWD "WiFi-wachtwoord"
#define LANG_ConfDMTZH2 "Domoticz-instellingen"
#define LANG_ConfDMTZBool "Verzenden naar Domoticz?"
#define LANG_ConfDMTZIP "IP-adres van Domoticz"
#define LANG_ConfDMTZPORT "Domoticz-poort"
#define LANG_ConfDMTZGIdx "Domoticz Gas Idx"
#define LANG_ConfDMTZEIdx "Domoticz Energy Idx"
#define LANG_ConfMQTTH2 "MQTT-instellingen"
#define LANG_ConfMQTTBool "MQTT-protocol inschakelen?"
#define LANG_ConfMQTTIP "IP-adres van MQTT-server"
#define LANG_ConfMQTTPORT "Poort van MQTT-server"
#define LANG_ConfMQTTUsr "MQTT-gebruiker"
#define LANG_ConfMQTTPSW "MQTT-wachtwoord"
#define LANG_ConfMQTTRoot "MQTT-hoofdonderwerp"
#define LANG_ConfReadP1Intr "Meetinterval in seconden"
#define LANG_ConfPERMUTTARIF "Peak/off-peak wisselen"
#define LANG_ConfTLNETH2 "Telnet-instellingen"
#define LANG_ConfTLNETBool "Telnet-poort activeren (23)"
#define LANG_ConfTLNETDBG "Debug via Telnet?"
#define LANG_ConfTLNETREPPORT "Stuur het Datagram via Telnet"
#define LANG_ConfMQTTDBG "Debug via MQTT?"
#define LANG_ConfSave "Opslaan"
#define LANG_OTAH1 "Firmware bijwerken"
#define LANG_OTAFIRMWARE "Firmware"
#define LANG_OTABTUPDATE "Bijwerken"
#define LANG_OTANSUCCESSNOK "Er was een probleem met de update"
#define LANG_OTANSUCCESSOK "De firmware is succesvol bijgewerkt."
#define LANG_OTASUCCESS1 "De module zal herstarten. Dit duurt ongeveer 30 seconden."
#define LANG_OTASUCCESS2 "De blauwe LED zal twee keer knipperen zodra de module is opgestart."
#define LANG_OTASUCCESS3 "De LED zal langzaam knipperen terwijl er verbinding wordt gemaakt met uw WiFi-netwerk."
#define LANG_OTASUCCESS4 "Als de blauwe LED blijft branden, is de configuratie mislukt en moet u opnieuw verbinding maken met het WiFi-netwerk <b>%s</b>"
#define LANG_OTASTATUSOK "Firmware geschreven in geheugen"
#define LANG_DATAH1 "Gemeten waarden"
#define LANG_DATALastGet "Ontvangen om"
#define LANG_DATAFullL "Verbruik daluren: totaal"
#define LANG_DATAFullProdL "Productie daluren: totaal"
#define LANG_DATACurAmp "Huidig stroomverbruik"
#define LANG_DATAFullH "Verbruik piekuren: totaal"
#define LANG_DATAFullProdH "Productie piekuren: totaal"
#define LANG_DATACurProdAmp "Huidige stroomproductie"
#define LANG_DATAUL1 "Spanning: L1"
#define LANG_DATAUL2 "Spanning: L2"
#define LANG_DATAUL3 "Spanning: L3"
#define LANG_DATAAL1 "Ampèrage: L1"
#define LANG_DATAAL2 "Ampèrage: L2"
#define LANG_DATAAL3 "Ampèrage: L3"
#define LANG_DATAGFull "Gasverbruik: totaal"
#define LANG_SHOWJSON "Toon JSON"
#define LANG_SHOWRAW "Toon datagram"
#define LANG_HLPH1 "Hulp"
#define LANG_H1ADMIN "Authenticatie"
#define LANG_PSWDLOGIN "Gebruikersnaam"
#define LANG_ADMINPSD "Wachtwoord"
#define LANG_ACTIONLOGIN "Inloggen"
#define LANG_H1WRONGPSD "Fout"
#define LANG_WRONGPSDBACK "Terug"
#define LANG_RF_RESTARTH1 "Actie succesvol"
#define LANG_RF_RESTTXT "Maak opnieuw verbinding met het WiFi van de module zodra deze opnieuw is opgestart"
#define LANG_ASKCONFIRM "Weet u zeker dat u deze actie wilt uitvoeren?"
#define LANG_TXTREBOOTPAGE "Herstart aangevraagd door gebruiker"
#endif
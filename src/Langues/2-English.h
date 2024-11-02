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

#if LANGUAGE == 2
#define LANG_LOADED //detect on compilation if the language was correctly set
#define LANG_HEADERLG "en"
#define LANG_H1Welcome "Please set a password"
#define LANG_PSWD1 "Password"
#define LANG_PSWD2 "Retype password"
#define LANG_PSWDERROR "Passwords do not match."
#define LANG_MENU "Menu"
#define LANG_MENUP1 "Measured values"
#define LANG_MENUGraph24 "24h Graph"
#define LANG_MENUConf "Configuration"
#define LANG_MENUOTA "Update firmware"
#define LANG_MENURESET "Factory reset"
#define LANG_MENUREBOOT "Reboot"
#define LANG_MENUPASSWORD "Credentials"
#define LANG_H1DATA "Data"
#define LANG_ConfH1 "WiFi and module configuration"
#define LANG_ACTIONSAVE "Save"
#define LANG_Conf_Saved "Settings have been successfully saved."
#define LANG_ConfReboot "The module will now restart. This will take about a minute."
#define LANG_ConfLedStart "The blue LED will blink twice once the module has finished booting."
#define LANG_ConfLedError "If the blue LED stays on, the setting has failed. Reconnect to the WiFi network '{#HOSTNAME#}' to correct the settings."
#define LANG_ConfP1H2 "Meter options"
#define LANG_ConfWIFIH2 "WiFi settings"
#define LANG_ConfSSID "SSID"
#define LANG_ConfWIFIPWD "WiFi password"
#define LANG_ConfDMTZH2 "Domoticz settings"
#define LANG_ConfDMTZBool "Send to Domoticz?"
#define LANG_ConfDMTZIP "Domoticz IP address"
#define LANG_ConfDMTZPORT "Domoticz port"
#define LANG_ConfDMTZGIdx "Domoticz Gas Idx"
#define LANG_ConfDMTZEIdx "Domoticz Energy Idx"
#define LANG_ConfMQTTH2 "MQTT settings"
#define LANG_ConfMQTTBool "Enable MQTT protocol?"
#define LANG_ConfMQTTIP "MQTT server IP address"
#define LANG_ConfMQTTPORT "MQTT server port"
#define LANG_ConfMQTTUsr "MQTT user"
#define LANG_ConfMQTTPSW "MQTT password"
#define LANG_ConfMQTTRoot "MQTT root topic"
#define LANG_ConfReadP1Intr "Measurement interval (sec)"
#define LANG_ConfPERMUTTARIF "Reverse peak/off-peak"
#define LANG_ConfTLNETH2 "Telnet settings"
#define LANG_ConfTLNETBool "Enable Telnet port (23)"
#define LANG_ConfTLNETDBG "Debug via Telnet?"
#define LANG_ConfMQTTDBG "Debug via MQTT?"
#define LANG_ConfSave "Save"
#define LANG_OTAH1 "Update firmware"
#define LANG_OTAFIRMWARE "Firmware"
#define LANG_OTABTUPDATE "Update"
#define LANG_OTANSUCCESSNOK "There was an issue with the update"
#define LANG_OTANSUCCESSOK "The firmware has been successfully updated."
#define LANG_OTASUCCESS1 "The module will restart. This will take about 30 seconds."
#define LANG_OTASUCCESS2 "The blue LED will blink twice once the module has finished booting."
#define LANG_OTASUCCESS3 "The LED will blink slowly while connecting to your WiFi network."
#define LANG_OTASUCCESS4 "If the blue LED stays on, the configuration has failed and you will need to reconnect to the WiFi network '{#HOSTNAME#}'"
#define LANG_DATAH1 "Measured values"
#define LANG_DATALastGet "Received at"
#define LANG_DATAFullL "Consumption off-peak: total"
#define LANG_DATAFullProdL "Production off-peak: total"
#define LANG_DATACurAmp "Current consumption"
#define LANG_DATAFullH "Consumption peak: total"
#define LANG_DATAFullProdH "Production peak: total"
#define LANG_DATACurProdAmp "Current production"
#define LANG_DATAUL1 "Voltage: L1"
#define LANG_DATAUL2 "Voltage: L2"
#define LANG_DATAUL3 "Voltage: L3"
#define LANG_DATAAL1 "Current: L1"
#define LANG_DATAAL2 "Current: L2"
#define LANG_DATAAL3 "Current: L3"
#define LANG_DATAGFull "Gas consumption: total"
#define LANG_SHOWJSON "Show JSON"
#define LANG_SHOWRAW "Show datagram"
#define LANG_HLPH1 "Help"
#define LANG_H1ADMIN "Authentication"
#define LANG_PSWDLOGIN "Username"
#define LANG_ADMINPSD "Password"
#define LANG_ACTIONLOGIN "Login"
#define LANG_H1WRONGPSD "Error"
#define LANG_WRONGPSDBACK "Back"
#define LANG_RF_RESTARTH1 "Operation successful"
#define LANG_RF_RESTTXT "Please reconnect to the module WiFi once it has restarted"
#define LANG_ASKCONFIRM "Are you sure you want to proceed with this action?"
#define LANG_TXTREBOOTPAGE "Reboot requested by user"
#endif

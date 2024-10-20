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

#ifndef LANG_H
#define LANG_H

#include <Arduino.h>
#include "Langues/French.h"
#include "Debug.h"

class Language
{
  public:
    const char* getLangValue(const char* key);
    void FindAndTranslateAll(String& inputText);
};

#ifdef NEDERLANDS

#define TIMESTAMP "%d dagen %d uren %d minuten"
#define LAST_SAMPLE " laatste sample: "
#define FIRMWARE_REV " firmware versie: "

#define MAIN_METER_VALUES "<form action='/P1' method='post'><button type='p1' class='button bhome'>Meterdata</button></form>"

#define MAIN_SETUP_TITLE "<fieldset><legend><b> Setup </b></legend>"
#define MAIN_SETUP "<form action='/Setup' method='post'><button type='Setup'>Setup</button></form>"
#define MAIN_UPDATE ""

#define SETUP_SAVED_TITLE "<fieldset><legend><b>Wifi en module setup</b></legend>"
#define SETUP_SAVED_TEXT  "<p><b>De instellingen zijn succesvol bewaard</b><br><br>De module zal nu herstarten. \
  Dat duurt ongeveer een minuut.</p><br><p>De led zal langzaam knipperen tijdens koppelen aan uw WiFi netwerk.</p><p>\
  Als de blauwe led blijft branden is de instelling mislukt en zult u opnieuw moeten koppelen met Wifi netwerk 'P1_Setup'.</p>"

#define FIRMWARE_UPDATE_SUCCESS_TITLE "<fieldset><legend><b>Firmware update</b></legend>"
#define FIRMWARE_UPDATE_SUCCESS_TEXT "<p><b>De firmware is succesvol bijgewerkt</b><br><br><p>De module zal nu herstarten. \
  Dat duurt ongeveer 30 sec</p><p>De blauwe Led zal 2x oplichten wanneer de module klaar is met opstarten</p><p>\
  De led zal langzaam knipperen tijdens koppelen aan uw WiFi netwerk.</p><p>Als de blauwe led blijft branden is de instelling mislukt \
  en zult u opnieuw moeten koppelen met Wifi netwerk 'P1_Setup'.</p>"

#define HELP_TITLE "<fieldset><legend><b>Help</b></legend>"
#define HELP_TEXT1 "<p><b>De P1 wifi-gateway kan op verschillende manieren data afleveren.</b><br><br><p>Altijd beschikbaar zijn de kernwaarden via <b>meterstanden</b> of via"
#define HELP_RAW "<p>De ruwe data is beschikbaar via "
#define HELP_TEXT2 "<p>Meer gangbaar is het gebruik van de gateway met een domotica systeem.</p>\
    <p><b>Domoticz</b> accepteert json berichten. Geef hiertoe het IP adres van <br>\
    je Domoticz server op en het poortnummer waarop deze kan worden benaderd (doorgaans 8080).</p>\
    De benodigde Idx voor gas en electra verkrijgt u door eerst in Domoticz virtule sensors \
    voor beiden aan te maken. Na creatie verschijnen de Idxen onder het tabblad 'devices'.</p><br><br>\
    Data kan ook (door Domoticz bijvoorbeeld) worden uitgelezen via poort 23 van de module (p1wifi.local:23).\
    Dit is de manier waarop Domoticz hardware device [P1 Smart Meter with LAN interface] data kan ophalen.\
    Aan de p1wifi kant hoeft daarvoor niets te worden ingesteld (geen vinkjes bij json en mqtt).<br><br>\
    Voor andere systemen kan je gebruik maken van een MQTT broker. Vul de gegevens van de broker in, en stel het root topic in. \
    Voor Home Assistant is 'dsmr' handig omdat dan de 'DSMR Reader' integration in HA kan worden gebruikt. Die verweerkt de data van \
    de module keurig.</p>\
    Geef met de checkboxes aan welke rapportage methode(n) je wilt gebruiken.</p>"


#endif


#ifdef GERMAN
#define HELP "/Help' target='_blank'>Hilfe</a>"
#define TIMESTAMP "%d Tagen %d Uhren %d Minuten"
#define LAST_SAMPLE " letztes sample: "
#define FIRMWARE_REV " firmware versie: "

#define MAIN_METER_VALUES

#define MAIN_SETUP
#define MAIN_UPDATE


#define SETUP_SAVED_TITLE
#define SETUP_SAVED_TEXT

#define FIRMWARE_UPDATE_SUCCESS_TITLE
#define FIRMWARE_UPDATE_SUCCESS_TEXT

#endif


#ifdef SWEDISH
#define HELP "/Help' target='_blank'>Hjälp</a>"
#define TIMESTAMP "%d dagar %d timmar %d minuter"
#define LAST_SAMPLE " laatste sample: "
#define FIRMWARE_REV " firmware versie: "


#define MAIN_METER_VALUES "<form action='/P1' method='post'><button type='p1' class='button bhome'>Mätardata</button></form>"

#define MAIN_SETUP_TITLE "<fieldset><legend><b> Uppstart </b></legend>"
#define MAIN_SETUP "<form action='/Setup' method='post'><button type='Setup'>Uppstart</button></form>"
#define MAIN_UPDATE "<form action='/Update' method='GET'><button type='submit'>Uppdatera firmware</button></fieldset></form>"


#define SETUP_SAVED_TITLE "<fieldset><legend><b>Wifi och moduluppställning</b></legend>"
#define SETUP_SAVED_TEXT "<p><b>Inställningarna har sparats.</b><br><br><p>Modulen kommer nu att starta om. Det tar ungefär en minut.</p><br><p>\
    Den blå lysdioden tänds 2x när modulen har startat klart.</p><p>Lysdioden blinkar långsamt när den ansluter till ditt WiFi-nätverk.</p><p>\
    Om den blå lysdioden fortsätter att lysa har inställningen misslyckats och du måste återansluta till WiFi-nätverket 'P1_Setup'.</p>"

#define FIRMWARE_UPDATE_SUCCESS_TITLE
#define FIRMWARE_UPDATE_SUCCESS_TEXT

#endif
#endif
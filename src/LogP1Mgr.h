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
#ifndef LOGP1MGR_H
#define LOGP1MGR_H

#define FILENAME_LAST24H "/Last24H.json"

#include <LittleFS.h>
#include <ArduinoJson.h>
#include "Debug.h"
#include "GlobalVar.h"
#include "P1Reader.h"

class LogP1Mgr
{
public:

  /// @brief Formatage et initiation de LittleFS
  void format()
  {
    LittleFS.format();
    LittleFS.begin();
  }

  explicit LogP1Mgr(settings &currentConf, P1Reader &currentP1) : DataReaderP1(currentP1)
  {
    if (!LittleFS.begin())
    {
      format();
    }
    MainSendDebug("[STRG] Ready");

    //Ecoute de nouveau datagram
    DataReaderP1.OnNewDatagram([this]()
    {
      newDataGram();
    });

    //LittleFS.remove(FILENAME_LAST24H); // for debug
  }

  /// @brief Conversion d'une chaîne hex en uint8_t
  /// @param chaine Valeur à convertir
  /// @return une valeur numérique non signé
  static uint8_t hexStringToUint8(const char* chaine)
  {
    uint8_t resultat = 0;
    int i = 0;

    // On parcourt la chaîne de caractères jusqu'à rencontrer un caractère non numérique ou la fin de la chaîne
    while (chaine[i] >= '0' && chaine[i] <= '9')
    {
      // On extrait le chiffre et on le multiplie par la puissance de 10 correspondante
      resultat = resultat * 10 + (chaine[i] - '0');
      i++;
    }

    return resultat;
  }
private:
  P1Reader &DataReaderP1;
  bool FileInitied = false;
  uint8_t LastHourInLast24H = 0;
  struct DateTime
  {
    uint8_t day;
    uint8_t month;
    uint8_t year;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
  };


  /// @brief 
  /// @param FileName Permet de charger un fichier JSON
  /// @param doc Le pointeur de l'élément où seront charger les données
  /// @return True si le fichier est chargé, sinon, False
  bool loadJSON(const char *FileName, JsonDocument &doc)
  {
    File file = LittleFS.open(FileName, "r");
    if(file)
    {
      DeserializationError error = deserializeJson(doc, file);
      file.close();
      
      if (!error)
      {
        return true;
      }
    }

    MainSendDebugPrintf("[STKG] Error on load %s", FileName);
    return false;
  }

  /// @brief Ouvre le fichier JSON et charge l'heure de la dernière mesure 
  void prepareLogLast24H()
  {
    JsonDocument doc;
    char datetime[13];
    
    FileInitied = true;
    if (!loadJSON(FILENAME_LAST24H, doc))
    {
      return;
    }

    JsonArray array = doc.as<JsonArray>();
    if (array.isNull())
    {
      return;
    }

    JsonObject lastItem = array[array.size()-1];
    strcpy(datetime, lastItem["DateTime"]);
    char charhour[2] = { datetime[6], datetime[7] };
    LastHourInLast24H = hexStringToUint8(charhour);
  }


  /// @brief Traitement d'une nouvelle mesure reçue
  void newDataGram()
  {
    char charhour[2] = { DataReaderP1.DataReaded.P1timestamp[6], DataReaderP1.DataReaded.P1timestamp[7] };
    uint8_t hour = hexStringToUint8(charhour);
    
    if (!FileInitied)
    {
      prepareLogLast24H();
    }

    if (LastHourInLast24H == hour)
    {
      return; // on attend l'heure prochaine !
    }

    writeNewLineInLast24H();
  }

  /// @brief Traitement du nouveau point et vérifie qu'il n'a pas trop de point dans le fichier JSON
  void writeNewLineInLast24H()
  {
    MainSendDebug("[STKG] Write log for 24H");
    JsonDocument Points;

    loadJSON(FILENAME_LAST24H, Points);

    if (Points.size() > 23)
    {
      u_int8_t NbrToRemove = Points.size() - 23;

      JsonDocument newDoc;
      for (size_t i = NbrToRemove; i <= Points.size(); i++)
      {
        newDoc.add(Points[i]);
      }

      addPointAndSave(newDoc);
      return;
    }

    addPointAndSave(Points);
  }

  /// @brief Ajoute et sauvegarde le fichier de mesure
  /// @param Points Les points de mesure actuel
  void addPointAndSave(JsonDocument Points)
  {
    JsonObject point = Points.add<JsonObject>();
    point["DateTime"] = DataReaderP1.DataReaded.P1timestamp;
    point["T1"] = DataReaderP1.DataReaded.electricityUsedTariff1.val();
    point["T2"] = DataReaderP1.DataReaded.electricityUsedTariff2.val();
    point["R1"] = DataReaderP1.DataReaded.electricityReturnedTariff1.val();
    point["R2"] = DataReaderP1.DataReaded.electricityReturnedTariff2.val();

    File file = LittleFS.open(FILENAME_LAST24H, "w");
    serializeJson(Points, file);
    file.close();
    
    //sauvegarde la derniére heure
    char charhour[2] = { DataReaderP1.DataReaded.P1timestamp[6], DataReaderP1.DataReaded.P1timestamp[7] };
    LastHourInLast24H = hexStringToUint8(charhour);
  }
};
#endif
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
#ifndef LOGP1MGR_H
#define LOGP1MGR_H

#define FILENAME_LAST24H "/Last24H.json"
#define MAX_POINTS 24

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

    // Écoute de nouveau datagram
    DataReaderP1.OnNewDatagram([this]()
                               { newDataGram(); });
  }

  /// @brief Conversion optimisée d'une chaîne numérique en uint8_t
  /// @param str Valeur à convertir (2 caractères max)
  /// @return une valeur numérique non signée
  static inline uint8_t fastParseUint8(const char *str)
  {
    return (str[0] - '0') * 10 + (str[1] - '0');
  }

private:
  P1Reader &DataReaderP1;
  bool FileInitied = false;
  uint8_t LastHourInLast24H = 0;

  /// @brief Charge seulement la dernière heure du fichier JSON
  /// @return True si succès
  bool loadLastHour()
  {
    File file = LittleFS.open(FILENAME_LAST24H, "r");
    if (!file)
      return false;

    // Lecture optimisée : chercher la dernière occurrence de "DateTime"
    file.seek(-200, SeekEnd); // Lire les 200 derniers caractères
    String content = file.readString();
    file.close();

    int lastDateTime = content.lastIndexOf("\"DateTime\":\"");
    if (lastDateTime != -1)
    {
      int startPos = lastDateTime + 12; // Longueur de "DateTime":""
      if (startPos + 6 < (int)content.length())
      {
        LastHourInLast24H = fastParseUint8(content.c_str() + startPos + 6);
        return true;
      }
    }
    return false;
  }

  /// @brief Traitement d'une nouvelle mesure reçue
  void newDataGram()
  {
    uint8_t currentHour = fastParseUint8(&DataReaderP1.DataReaded.P1timestamp[6]);

    if (!FileInitied)
    {
      FileInitied = true;
      loadLastHour();
    }

    if (LastHourInLast24H == currentHour)
    {
      return; // On attend l'heure prochaine !
    }

    writeNewLineInLast24H();
    LastHourInLast24H = currentHour;
  }

  /// @brief Écriture optimisée avec gestion de taille
  void writeNewLineInLast24H()
  {
    MainSendDebug("[STKG] Write log for 24H");

    // Utiliser JsonDocument moderne sans calcul de capacité
    JsonDocument doc;

    // Charger le fichier existant
    File file = LittleFS.open(FILENAME_LAST24H, "r");
    if (file)
    {
      DeserializationError error = deserializeJson(doc, file);
      file.close();

      if (error)
      {
        MainSendDebugPrintf("[STKG] JSON parse error: %s", error.c_str());
        doc.clear(); // Recommencer avec un document vide
      }
    }

    // S'assurer qu'on a un array
    if (!doc.is<JsonArray>())
    {
      doc.to<JsonArray>();
    }

    JsonArray array = doc.as<JsonArray>();

    // Supprimer les anciens points si nécessaire
    while (array.size() >= MAX_POINTS)
    {
      array.remove(0);
    }

    // Ajouter le nouveau point avec la syntaxe moderne
    JsonObject point = array.add<JsonObject>();
    point["DateTime"] = DataReaderP1.DataReaded.P1timestamp;
    point["T1"] = DataReaderP1.DataReaded.electricityUsedTariff1.val();
    point["T2"] = DataReaderP1.DataReaded.electricityUsedTariff2.val();
    point["R1"] = DataReaderP1.DataReaded.electricityReturnedTariff1.val();
    point["R2"] = DataReaderP1.DataReaded.electricityReturnedTariff2.val();

    // Sauvegarder atomiquement
    File outFile = LittleFS.open(FILENAME_LAST24H, "w");
    if (outFile)
    {
      serializeJson(doc, outFile);
      outFile.close();
    }
    else
    {
      MainSendDebug("[STKG] Error: Cannot write file");
    }
  }
};

#endif
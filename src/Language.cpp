
/*
 * Copyright (c) 2022 Ronald Leenes
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

#include "Language.h"

const char* Language::getLangValue(const char* key)
{
  const char* start = strstr(LANG, key);
  if (start != NULL)
  {
    start = strchr(start, ':');
    if (start != NULL) {
      start = strchr(start, '"');
      if (start != NULL)
      {
        start++;
        const char* end = strchr(start, '"');
        if (end != NULL)
        {
          int length = end - start;
          char* value = new char[length + 1]; // Utilisation de la mémoire dynamique
          strncpy(value, start, length);
          value[length] = '\0';
          return value;
        }
      }
    }
  }
  return "";
}

char* Language::FindAndTranslateAll(const char *atraduire)
{
  // Créer une copie de la chaîne source pour la manipulation
  String inputText = atraduire;
  int startIndex = inputText.indexOf("{-");
  int endIndex = inputText.indexOf("-}");

  // Remplacement des balises de type {-XYZ-}
  while (startIndex != -1 && endIndex != -1)
  {
    String textBetweenBraces = inputText.substring(startIndex + 2, endIndex);
    inputText.replace("{-" + textBetweenBraces + "-}", getLangValue(textBetweenBraces.c_str()));

    // Recherche des prochaines occurrences
    startIndex = inputText.indexOf("{-");
    endIndex = inputText.indexOf("-}");
  }

  // Ajouter d'autres remplacements si nécessaire
  inputText.replace("{#HOSTNAME#}", GetClientName());

  // Allouer dynamiquement un tableau de caractères pour le résultat
  size_t resultSize = inputText.length() + 1; // +1 pour le caractère nul de fin
  char* result = (char*)malloc(resultSize);
  if (result != nullptr)
  {
    // Copier le texte traduit dans le tableau alloué
    strcpy(result, inputText.c_str());
  }

  // Retourner le pointeur vers le tableau alloué
  return result;
}
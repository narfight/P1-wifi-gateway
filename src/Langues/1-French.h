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
 
 * Additionally, please note that the original source code of this file
 * may contain portions of code derived from (or inspired by)
 * previous works by:
 *
 * Ronald Leenes (https://github.com/romix123/P1-wifi-gateway and http://esp8266thingies.nl)
 * MQTT part based on https://github.com/daniel-jong/esp8266_p1meter/blob/master/esp8266_p1meter/esp8266_p1meter.ino
 */

#if LANGUAGE == 1
#define LANG_LOADED //detect on compilation if the language was good
#define LANG_HEADERLG "fr"
#define LANG_H1Welcome "Merci de définir un mot de passe"
#define LANG_PSWD1 "Mot de passe"
#define LANG_PSWD2 "le retaper"
#define LANG_PSWDERROR "Les deux mot de passe ne sont pas identique."
#define LANG_MENU "Menu"
#define LANG_MENUP1 "Valeurs mesurées"
#define LANG_MENUGraph24 "Graphique sur 24h"
#define LANG_MENUConf "Configuration"
#define LANG_MENUOTA "Mettre à jour le micrologiciel"
#define LANG_MENURESET "Retour aux paramètres d'usine"
#define LANG_MENUREBOOT "Redémarrer"
#define LANG_MENUPASSWORD "Identifiant"
#define LANG_H1DATA "Données"
#define LANG_ConfH1 "Wifi et configuration du module"
#define LANG_ACTIONSAVE "Sauvegarder"
#define LANG_Conf_Saved "Les paramètres ont été enregistrés avec succès."
#define LANG_ConfReboot "Le module va maintenant redémarrer. Cela prendra environ une minute."
#define LANG_ConfLedStart "La Led bleue s'allumera 2x lorsque le module aura fini de démarrer."
#define LANG_ConfLedError "Si la LED bleue reste allumée, c'est que le réglage a échoué. Reconnectez vous alors au réseau WiFi <b>%s</b> pour corriger les paramètres."
#define LANG_ConfP1H2 "Option sur le compteur"
#define LANG_ConfWIFIH2 "Paramètres Wi-Fi"
#define LANG_ConfSSID "SSID"
#define LANG_ConfWIFIPWD "Mot de passe"
#define LANG_ConfDMTZH2 "Paramètres Domoticz"
#define LANG_ConfDMTZBool "Envoyer à Domoticz ?"
#define LANG_ConfDMTZIP "Adresse IP Domoticz"
#define LANG_ConfDMTZPORT "Port Domoticz"
#define LANG_ConfDMTZGIdx "Domoticz Gaz Idx"
#define LANG_ConfDMTZEIdx "Domoticz Energy Idx"
#define LANG_ConfMQTTH2 "Paramètres MQTT"
#define LANG_ConfMQTTBool "Activer le protocole MQTT ?"
#define LANG_ConfMQTTIP "Adresse IP du serveur MQTT"
#define LANG_ConfMQTTPORT "Port du serveur MQTT"
#define LANG_ConfMQTTUsr "Utilisateur MQTT"
#define LANG_ConfMQTTPSW "Mot de passe MQTT"
#define LANG_ConfMQTTRoot "Rubrique racine MQTT"
#define LANG_ConfReadP1Intr "Intervalle de mesure en sec"
#define LANG_ConfPERMUTTARIF "Inverser heure creuse/pleine"
#define LANG_ConfTLNETH2 "Paramètres Telnet"
#define LANG_ConfTLNETBool "Activer le port Telnet (23)"
#define LANG_ConfTLNETDBG "Debug via Telnet ?"
#define LANG_ConfTLNETREPPORT "Envoie le Datagram sur Telnet"
#define LANG_ConfMQTTDBG "Debug via MQTT ?"
#define LANG_ConfSave "Sauvegarder"
#define LANG_OTAH1 "Mettre à jour le micrologiciel"
#define LANG_OTAFIRMWARE "Micrologiciel"
#define LANG_OTABTUPDATE "Mettre à jour"
#define LANG_OTANSUCCESSNOK "Il a eu un problème avec la mise à jour"
#define LANG_OTANSUCCESSOK "Le micrologiciel a été mis à jour avec succès."
#define LANG_OTASUCCESS1 "Le module va redémarrer. Cela prend environ 30 secondes."
#define LANG_OTASUCCESS2 "La LED bleue s'allumera deux fois une fois que le module aura terminé son démarrage."
#define LANG_OTASUCCESS3 "La LED clignotera lentement pendant la connexion à votre réseau WiFi."
#define LANG_OTASUCCESS4 "Si la LED bleue reste allumée, la configuration a échoué et vous devrez refaire la connexion avec le réseau WiFi <b>%s</b>"
#define LANG_OTASTATUSOK "Micrologiciel écrit en mémoire"
#define LANG_DATAH1 "Valeurs mesurées"
#define LANG_DATALastGet "Recue à"
#define LANG_DATAFullL "Consommation heures creuses: total"
#define LANG_DATAFullProdL "Production heures creuses: total"
#define LANG_DATACurAmp "Consommation instantanée de courant"
#define LANG_DATAFullH "Consommation heures pleines: total"
#define LANG_DATAFullProdH "Production heures pleines: total"
#define LANG_DATACurProdAmp "Production instantanée de courant"
#define LANG_DATAUL1 "Tension: L1"
#define LANG_DATAUL2 "Tension: L2"
#define LANG_DATAUL3 "Tension: L3"
#define LANG_DATAAL1 "Ampérage: L1"
#define LANG_DATAAL2 "Ampérage: L2"
#define LANG_DATAAL3 "Ampérage: L3"
#define LANG_DATAGFull "Consommation de gaz: total"
#define LANG_SHOWJSON "Afficher le Json"
#define LANG_SHOWRAW "Afficher le datagram"
#define LANG_HLPH1 "Aide"
#define LANG_H1ADMIN "Identification"
#define LANG_PSWDLOGIN "Identifiant"
#define LANG_ADMINPSD "Mot de passe"
#define LANG_ACTIONLOGIN "Se connecter"
#define LANG_H1WRONGPSD "Erreur"
#define LANG_WRONGPSDBACK "Retour"
#define LANG_RF_RESTARTH1 "Opération réussie"
#define LANG_RF_RESTTXT "Merci de vous connecter au Wifi du module une fois qu'il aura redémarré"
#define LANG_ASKCONFIRM "Êtes-vous sûr de vouloir réaliser cette action ?"
#define LANG_TXTREBOOTPAGE "Redémarrage demandé par l'utilisateur"
#endif
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

#include "HTTPMgr.h"

HTTPMgr::HTTPMgr(settings &currentConf, TelnetMgr &currentTelnet, MQTTMgr &currentMQTT, P1Reader &currentP1) : conf(currentConf), TelnetSrv(currentTelnet), MQTT(currentMQTT), P1Captor(currentP1), server(80)
{
}

void HTTPMgr::start_webservices()
{
  MainSendDebugPrintf("[WWW] Start on port %d", WWW_PORT_HTTP);
  MDNS.begin(GetClientName());
  //header files
  server.on("/style.css", std::bind(&HTTPMgr::handleStyleCSS, this));
  server.on("/favicon.svg", std::bind(&HTTPMgr::handleFavicon, this));
  server.on("/main.js", std::bind(&HTTPMgr::handleMainJS, this));
  
  //extra for /P1 page for refresh
  server.on("/P1.json", std::bind(&HTTPMgr::handleJSON, this));
  server.on("/P1.js", std::bind(&HTTPMgr::handleP1Js, this));
  
  //for the footer
  server.on("/status.json", std::bind(&HTTPMgr::handleJSONStatus, this));

  //pages
  server.on("/", std::bind(&HTTPMgr::handleRoot, this));
  server.on("/setPassword", std::bind(&HTTPMgr::handlePassword, this));
  server.on("/Setup", std::bind(&HTTPMgr::handleSetup, this));
  server.on("/SetupSave", std::bind(&HTTPMgr::handleSetupSave, this));
  server.on("/reset", std::bind(&HTTPMgr::handleFactoryReset, this));
  server.on("/reboot", std::bind(&HTTPMgr::handleReboot, this));
  server.on("/P1", std::bind(&HTTPMgr::handleP1, this));
  server.on("/raw", std::bind(&HTTPMgr::handleRAW, this));
  server.on("/Help", std::bind(&HTTPMgr::handleHelp, this));
  server.on("/update", HTTP_GET, std::bind(&HTTPMgr::handleUploadForm, this));
  server.on("/update", HTTP_POST, [this]()
  {
    if (!ChekifAsAdmin())
    {
      return;
    }

    if (UpdateResultFailed)
    {
      ReplyOTANOK(UpdateMsg.c_str(), UpdateErrorCode);
      UpdateResultFailed = false;
      UpdateMsg = "";
    }
    else
    {
        ReplyOTAOK();
    }
  }, std::bind(&HTTPMgr::handleUploadFlash, this));

  server.begin();
  MDNS.addService("http", "tcp", WWW_PORT_HTTP);
}


bool HTTPMgr::ActifCache(bool enabled)
{
  if (enabled)
  {
    //Gestion du cache sur base de la version du firmware
    String etag = "W/\"" + String(BUILD_DATE) + "\"";
    if (server.header("If-None-Match") == etag)
    {
      server.send(304);
      return true;
    }
    // Cache
    server.sendHeader("ETag", etag);
    server.sendHeader("Cache-Control", "max-age=86400");

    return false;
  }
  else
  {
    // Définir les en-têtes HTTP pour désactiver le cache
    server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
    server.sendHeader("Pragma", "no-cache");
    server.sendHeader("Expires", "-1");

    return false;
  }
}

void HTTPMgr::DoMe()
{
  server.handleClient();
  MDNS.update();
}

void HTTPMgr::handleRoot()
{
  // You cannot use this page if is not your first boot
  if (conf.NeedConfig)
  {
    server.sendHeader("Location", "/setPassword");
    server.send(302, "text/plain", "Redirecting");
    return;
  }

  String str = F("<main class='form-signin'>");
  str += F("<fieldset><legend>{-H1DATA-}</legend>");
  str += F("<a href=\"/P1\" class=\"bt\">{-MENUP1-}</a>");
  str += F("</fieldset>");
  str += F("<fieldset><legend>{-ConfH1-}</legend>");
  str += F("<a href=\"/Setup\" class=\"bt\">{-MENUConf-}</a>");
  str += F("<a href=\"/setPassword\" class=\"bt\">{-MENUPASSWORD-}</a>");
  str += F("<a href=\"/update\" class=\"bt\">{-MENUOTA-}</a>");
  str += F("<a href=\"/reboot\" class=\"bt bwarning\">{-MENUREBOOT-}</a>");
  str += F("<a href=\"/reset\" class=\"bt bwarning\">{-MENURESET-}</a>");
  str += F("</fieldset>");
  TradAndSend("text/html", str, "", false);
}

void HTTPMgr::ReplyOTAOK()
{
  MainSendDebug("[FLASH] Ok, reboot now");
  String str = F("<fieldset><p>{-OTASUCCESS1-}</p><p>{-OTASUCCESS2-}</p><p>{-OTASUCCESS3-}</p><p>{-OTASUCCESS4-}</p><p>{-OTASUCCESS5-}</p>");
  str += GetAnimWait();
  str += F("</fieldset>");
  TradAndSend("text/html", str, "", true);
  RequestRestart(1000);
}

void HTTPMgr::ReplyOTANOK(String Error, u_int ref)
{
  MainSendDebugPrintf("[FLASH] Error : %s (%u)", Error.c_str(), ref);
  String str = F("<fieldset><p>{-OTANOTSUCCESS-} : <strong>") + Error + " (" + String(ref) + F(")</strong></p><p>{-OTASUCCESS2-}</p><p>{-OTASUCCESS3-}</p><p>{-OTASUCCESS4-}</p><p>{-OTASUCCESS5-}</p>");
  str += GetAnimWait();
  str += F("</fieldset>");
  TradAndSend("text/html", str, "", true);
  RequestRestart(1000);
}

void HTTPMgr::handleRAW()
{
  server.send(200, "Text/plain", P1Captor.datagram);
}

void HTTPMgr::handleP1Js()
{
  MainSendDebug("[HTTP] Request P1.js");

  if (ActifCache(true)) return;

  String str = F("async function updateValues(){try{let e=await fetch(\"P1.json\"),a=await e.json();document.getElementById(\"LastSample\").value=parseDateTime(a.LastSample).toLocaleString(),document.getElementById(\"electricityUsedTariff1\").value=a.DataReaded.electricityUsedTariff1+\" kWh\",document.getElementById(\"electricityUsedTariff2\").value=a.DataReaded.electricityUsedTariff2+\" kWh\",document.getElementById(\"electricityReturnedTariff1\").value=a.DataReaded.electricityReturnedTariff1+\" kWh\",document.getElementById(\"electricityReturnedTariff2\").value=a.DataReaded.electricityReturnedTariff2+\" kWh\",document.getElementById(\"actualElectricityPowerDeli\").value=a.DataReaded.actualElectricityPowerDeli+\" kWh\",document.getElementById(\"actualElectricityPowerRet\").value=a.DataReaded.actualElectricityPowerRet+\" kWh\",document.getElementById(\"instantaneousVoltageL1\").value=a.DataReaded.instantaneousVoltage.L1+\" V\",document.getElementById(\"instantaneousVoltageL2\").value=a.DataReaded.instantaneousVoltage.L2+\" V\",document.getElementById(\"instantaneousVoltageL3\").value=a.DataReaded.instantaneousVoltage.L3+\" V\",document.getElementById(\"instantaneousCurrentL1\").value=a.DataReaded.instantaneousCurrent.L1+\" A\",document.getElementById(\"instantaneousCurrentL2\").value=a.DataReaded.instantaneousCurrent.L2+\" A\",document.getElementById(\"instantaneousCurrentL3\").value=a.DataReaded.instantaneousCurrent.L3+\" A\",document.getElementById(\"gasReceived5min\").value=a.DataReaded.gasReceived5min+\" m3\"}catch(t){console.error(\"Error on update :\",t)}}setInterval(updateValues,1e4),window.onload=updateValues;");
  
  // no translate for CSS
  server.send(200, "application/javascript", str);
}

void HTTPMgr::handleStyleCSS()
{
  MainSendDebug("[HTTP] Request style.css");
  
  if (ActifCache(true)) return;

  String str = F("body {font-family: Verdana, sans-serif;background-color: #f9f9f9;margin: 0;padding: 20px;text-align: center;}");
  str += F(".container {max-width: 600px;margin: 0 auto;background: #ffffff;border-radius: 8px;box-shadow: 0 2px 10px rgba(0, 0, 0, 0.1);padding: 20px 20px 0px 20px;}");
  str += F("h2 {text-align:center;color:#000000;}");
  str += F("fieldset {border: 1px solid #ddd;border-radius: 8px;padding: 10px;margin-bottom: 20px;}");
  str += F("fieldset input {width: 30%;padding: 5px;border: 1px solid #ddd;border-radius: 4px;font-size: 1em;box-shadow: inset 0 1px 3px rgba(0, 0, 0, 0.1)}");
  str += F("legend {font-weight: bold;padding: 0 10px;font-size: 1.2em}");
  str += F("label {display: inline-block;text-align: right;width: 60%;text-align: right;margin-right: 10px;margin-bottom: 12px;font-weight: normal}");
  str += F(".help, .footer {text-align:right;font-size:11px;color:#aaa}");
  str += F("p {margin: 0.5em 0;}");
  str += F("button, .bt {display: inline-block;text-align: center;text-decoration: none;border: 0;border-radius: 0.3rem;background: #97C1A9;color: #ffffff;line-height: 2.4rem;font-size: 1.2rem;width: 100%;-webkit-transition-duration: 0.4s;transition-duration: 0.4s;cursor: pointer;margin-top: 5px;}");
  str += F("button:hover, .bt:hover {background: #0e70a4;}");
  str += F(".bt[href=\"/\"], .bt[href=\"/P1\"]{background: #55CBCD;}");
  str += F(".bt[href=\"/\"]:hover, .bt[href=\"/P1\"]:hover {background: #A2E1DB;}");
  str += F(".bwarning {background: #E74C3C;}");
  str += F(".bwarning:hover {background: #C0392B;}");
  str += F("a {color: #1fa3ec;text-decoration: none;}");
  str += F(".row:after {content: \"\";display: table; clear: both;}");
  str += F("svg {display: block;margin: auto;}");
  str += F(".status-bar {display: flex;justify-content: flex-end;margin-top: 10px;padding: 10px;border-top: 1px solid #ddd;}");
  str += F(".status-bar .indicator {width: 10px;height: 10px;border-radius: 50%;background-color: green;margin-right: 5px;display: inline-block;}");
  str += F(".error {background-color: red !important;}");
  str += F(".status-bar .text {margin-left: 5px;}");
  str += F(".status-bar .item {display: flex;align-items: center;margin-bottom: 5px;padding-left: 10px}");
  
  // no translate for CSS
  server.send(200, "text/css", str);
}
void HTTPMgr::handleMainJS()
{
  MainSendDebug("[HTTP] Request main.js");
  
  if (ActifCache(true)) return;

  String str = F("function parseDateTime(t){return new Date(\"20\"+t.substring(0,2),t.substring(2,4)-1,t.substring(4,6),t.substring(6,8),t.substring(8,10),t.substring(10,12))}async function updateStatus(){try{let e=await fetch(\"status.json\"),s=await e.json();const r=document.getElementById(\"MQTT-indicator\");null!=r&&(1==s.MQTT?r.classList.remove(\"error\"):r.classList.add(\"error\"));const n=document.getElementById(\"P1-indicator\");if(\"\"!=s.P1.LastSample){var t=parseDateTime(s.P1.LastSample);Date.now().set;t.setSeconds(t.getSeconds()+3*s.P1.Interval),t<Date.now()?n.classList.add(\"error\"):n.classList.remove(\"error\")}else n.classList.add(\"error\")}catch(t){console.error(\"Error on update status:\",t)}}window.onload=function(){updateStatus();document.querySelectorAll(\".bwarning\").forEach((t=>{t.addEventListener(\"click\",(function(t){confirm(\"{-ASKCONFIRM-}\")||t.preventDefault()}))})),setInterval(updateStatus,1e4)};");

  Trad.FindAndTranslateAll(str);
  server.send(200, "application/javascript", str);
}
void HTTPMgr::handleFavicon()
{
  MainSendDebug("[HTTP] Request favicon.svg");

  if (ActifCache(true)) return;

  String str = F("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
  str += F("<svg xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" version=\"1.1\" viewBox=\"0 0 24 24\">");
  str += F("<path d=\"M4,4H20A2,2 0 0,1 22,6V18A2,2 0 0,1 20,20H4A2,2 0 0,1 2,18V6A2,2 0 0,1 4,4M4,6V18H11V6H4M20,18V6H18.76C19,6.54 18.95,7.07 18.95,7.13C18.88,7.8 18.41,8.5 18.24,8.75L15.91,11.3L19.23,11.28L19.24,12.5L14.04,12.47L14,11.47C14,11.47 17.05,8.24 17.2,7.95C17.34,7.67 17.91,6 16.5,6C15.27,6.05 15.41,7.3 15.41,7.3L13.87,7.31C13.87,7.31 13.88,6.65 14.25,6H13V18H15.58L15.57,17.14L16.54,17.13C16.54,17.13 17.45,16.97 17.46,16.08C17.5,15.08 16.65,15.08 16.5,15.08C16.37,15.08 15.43,15.13 15.43,15.95H13.91C13.91,15.95 13.95,13.89 16.5,13.89C19.1,13.89 18.96,15.91 18.96,15.91C18.96,15.91 19,17.16 17.85,17.63L18.37,18H20M8.92,16H7.42V10.2L5.62,10.76V9.53L8.76,8.41H8.92V16Z\"/>");
  str += F("</svg>");
  
  // no translate for CSS
  server.send(200, "image/svg+xml", str);
}

void HTTPMgr::handleReboot()
{
  if (!ChekifAsAdmin())
  {
    return;
  }

  RebootPage("TXTREBOOTPAGE");
  RequestRestart(1000);  
}

void HTTPMgr::handleUploadForm()
{
  if (!ChekifAsAdmin())
  {
    return;
  }

  String str = F("<form method='post' action='' enctype='multipart/form-data'>");
  str += F("<fieldset><legend>{-OTAH1-}</legend>");
  str += F("<p><label for=\"firmware\">{-OTAFIRMWARE-} :</label><input type='file' accept='.bin,.bin.gz' id='firmware' name='firmware' required></p>");
  str += F("</fieldset><button class=\"bt bwarning\" type='submit'>{-OTABTUPDATE-}</button></form>");
  str += F("<a href=\"/\" class=\"bt\">{-MENU-}</a>");
  TradAndSend("text/html", str, "", false);
}

void HTTPMgr::handleUploadFlash()
{
  if (ChekifAsAdmin())
  {
    HTTPUpload& upload = server.upload();

    if (UpdateResultFailed)
    {
      //on a un souci avec cette mise à jour, on ignore l'upload
      return;
    }

    if (upload.status == UPLOAD_FILE_START)
    {
      Update.clearError();
      
      //check size en space
      uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
      MainSendDebugPrintf("[FLASH] Upload of '%s' (%lu octet)", upload.filename.c_str(), upload.contentLength);
      MainSendDebugPrintf("[FLASH] Space in memory : %lu octect", maxSketchSpace);

      if (upload.contentLength > maxSketchSpace)
      {
        UpdateResultFailed = true; // true = erreur d'update
        UpdateMsg = "Not enough space for update"; // Message d'erreur de la mise à jour
        UpdateErrorCode = 4;
        return;
      }

      if (!Update.begin(maxSketchSpace, U_FLASH))
      {
        UpdateResultFailed = true; // true = erreur d'update
        UpdateMsg = Update.getErrorString(); // Message d'erreur de la mise à jour
        UpdateErrorCode = 0;
        return;
      }
    }
    else if (upload.status == UPLOAD_FILE_WRITE)
    {
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
      {
        UpdateResultFailed = true; // true = erreur d'update
        UpdateMsg = Update.getErrorString(); // Message d'erreur de la mise à jour
        UpdateErrorCode = 1;
        return;
      }
    }
    else if (upload.status == UPLOAD_FILE_END)
    {
      if (Update.end(true)) // true to set the size to the current progress
      {
        //fin du flash, tout est bon :-)
        return;
      }
      else
      {
        UpdateResultFailed = true; // true = erreur d'update
        UpdateMsg = Update.getErrorString(); // Message d'erreur de la mise à jour
        UpdateErrorCode = 1;
        return;
      }
    }
    else if(upload.status == UPLOAD_FILE_ABORTED)
    {
        UpdateResultFailed = true; // true = erreur d'update
        UpdateMsg = "Update was aborted"; // Message d'erreur de la mise à jour
        UpdateErrorCode = 3;
        return;
    }
    yield();
  }
}

void HTTPMgr::handleFactoryReset()
{
  if (!ChekifAsAdmin())
  {
    return;
  }

  MainSendDebug("[HTTP] Reset factory and reboot...");
  RebootPage("RF_RESTTXT");

  conf.ConfigVersion = SETTINGVERSIONNULL;

  EEPROM.begin(sizeof(struct settings));
  EEPROM.put(0, conf);
  EEPROM.commit();
  
  RequestRestart(1000);
}

void HTTPMgr::handlePassword()
{
  if (!conf.NeedConfig) // if need config, don't ask password
  {
    if (!ChekifAsAdmin())
    {
      return;
    }
  }

  // Is the new password ?
  if (server.method() == HTTP_POST && server.hasArg("psd1") && server.hasArg("psd2"))
  {
    if (server.arg("psd1") == server.arg("psd2"))
    {
      conf.NeedConfig = false;
      server.arg("psd1").toCharArray(conf.adminPassword, sizeof(conf.adminPassword));
      server.arg("adminUser").toCharArray(conf.adminUser, sizeof(conf.adminUser));
      
      conf.BootFailed = 0;
      MainSendDebug("[HTTP] New admin password");
      EEPROM.begin(sizeof(struct settings));
      EEPROM.put(0, conf);
      EEPROM.commit();

      // Move to full setup !
      server.sendHeader("Location", "/");
      server.send(302, "text/plain", "Redirecting");
      return;
    }
  }

  String str = F("<form action=\"/setPassword\" method=\"post\" onsubmit=\"return Check()\">");
  str += F("<fieldset><legend>{-H1Welcome-}</legend>");
  str += F("<label for=\"adminUser\">{-PSWDLOGIN-} :</label><input type=\"text\" name=\"adminUser\" id=\"adminUser\" maxlength=\"32\" value='");
  str += nettoyerInputText(conf.adminUser);
  str += F("' \"><br />");
  str += F("<label for=\"psd1\">{-PSWD1-} :</label><input type=\"password\" name=\"psd1\" id=\"psd1\" maxlength=\"32\"><br />");
  str += F("<label for=\"psd2\">{-PSWD2-} :</label><input type=\"password\" name=\"psd2\" id=\"psd2\" maxlength=\"32\"><br />");
  str += F("<span id=\"passwordError\" class=\"error\"></span>");
  str += F("</fieldset><button type='submit'>{-ConfSave-}</button></form>");
  str += F("<a href=\"/\" class=\"bt\">{-MENU-}</a>");
  TradAndSend("text/html", str, "", false);
}

void HTTPMgr::handleSetup()
{
  if (!ChekifAsAdmin())
  {
    return;
  }

  String str = F("<form action='/SetupSave' method='post'>");
  str += F("<fieldset><legend>{-ConfWIFIH2-}</legend>");
  str += F("<label for=\"ssid\">{-ConfSSID-} :</label><input type='text' name='ssid' id='ssid' maxlength=\"32\" value='");
  str += nettoyerInputText(conf.ssid);
  str += F("'><br />");
  str += F("<label for=\"password\">{-ConfWIFIPWD-} :</label><input type='password' maxlength=\"64\" name='password' id='password' value='");
  str += nettoyerInputText(conf.password);
  str += F("'><br />");
  str += F("</fieldset>");

  str += F("<fieldset><legend>{-ConfDMTZH2-}</legend>");
  str += F("<label for=\"domo\">{-ConfDMTZBool-} :</label><input type='checkbox' name='domo' id='domo' ");

  if (conf.domo)
  {
    str += F(" checked><br />");
  }
  else
  {
    str += F("><br />");
  }
  str += F("<label for=\"domoticzIP\">{-ConfDMTZIP-} :</label><input type='text' name='domoticzIP' id='domoticzIP' maxlength=\"29\" value='");
  str += nettoyerInputText(conf.domoticzIP);
  str += F("'><br />");
  str += F("<label for=\"domoticzPort\">{-ConfDMTZPORT-} :</label><input type='number' min='1' max='65535' id='domoticzPort' name='domoticzPort' value='");
  str += conf.domoticzPort;
  str += F("'><br />");
  str += F("<label for=\"domoticzGasIdx\">{-ConfDMTZGIdx-} :</label><input type='number' min='1' id='domoticzGasIdx' name='domoticzGasIdx' value='");
  str += conf.domoticzGasIdx;
  str += F("'><br />");
  str += F("<label for=\"domoticzEnergyIdx\">{-ConfDMTZEIdx-} :</label><input type='number' min='1' id='domoticzEnergyIdx' name='domoticzEnergyIdx' value='");
  str += conf.domoticzEnergyIdx;
  str += F("'>");
  str += F("</fieldset>");

  str += F("<fieldset><legend>{-ConfMQTTH2-}</legend>");
  str += F("<label for=\"mqtt\">{-ConfMQTTBool-} :</label><input type='checkbox' name='mqtt' id='mqtt' ");
  if (conf.mqtt)
  {
    str += F(" checked></p>");
  }
  else
  {
    str += F("><br />");
  }

  str += F("<label for=\"mqttIP\">{-ConfMQTTIP-} :</label><input type='text' id='mqttIP' name='mqttIP' maxlength=\"29\" value='");
  str += nettoyerInputText(conf.mqttIP);
  str += F("'><br />");
  str += F("<label for=\"mqttPort\">{-ConfMQTTPORT-} :</label><input type='number' min='1' max='65535' id='mqttPort' name='mqttPort' value='");
  str += conf.mqttPort;
  str += F("'><br />");
  str += F("<label for=\"mqttUser\">{-ConfMQTTUsr-} :</label><input type='text' id='mqttUser' name='mqttUser' maxlength=\"31\" value='");
  str += nettoyerInputText(conf.mqttUser);
  str += F("'><br />");
  str += F("<label for=\"mqttPass\">{-ConfMQTTPSW-} :</label><input type='password' id='mqttPass' name='mqttPass' maxlength=\"31\" value='");
  str += nettoyerInputText(conf.mqttPass);
  str += F("'><br />");
  str += F("<label for=\"mqttTopic\">{-ConfMQTTRoot-} :</label><input type='text' id='mqttTopic' name='mqttTopic' maxlength=\"49\" value='");
  str += nettoyerInputText(conf.mqttTopic);
  str += F("'><br />");
  str += F("<label for=\"interval\">{-ConfMQTTIntr-} :</label><input type='number' min='10' id='interval' name='interval' value='");
  str += conf.interval;
  str += F("'><br />");
  str += F("<label for=\"InvTarif\">{-ConfPERMUTTARIF-} :</label><input type='checkbox' name='InvTarif' id='InvTarif' ");
  if (conf.InverseHigh_1_2_Tarif)
  {
    str += F(" checked></p>");
  }
  else
  {
    str += F("><br />");
  }
  str += F("<label for=\"debugToMqtt\">{-ConfMQTTDBG-} :</label><input type='checkbox' name='debugToMqtt' id='debugToMqtt' ");
  if (conf.debugToMqtt)
  {
    str += F(" checked><br />");
  }
  else
  {
    str += F("><br />");
  }
  str += F("</fieldset><fieldset><legend>{-ConfTLNETH2-}</legend>");
  str += F("<label for=\"telnet\">{-ConfTLNETBool-} :</label><input type='checkbox' name='telnet' id='telnet' ");
  if (conf.telnet)
  {
    str += F(" checked><br />");
  }
  else
  {
    str += F("><br />");
  }
  str += F("<label for=\"debugToTelnet\">{-ConfTLNETDBG-} :</label><input type='checkbox' name='debugToTelnet' id='debugToTelnet' ");
  if (conf.debugToTelnet)
  {
    str += F(" checked><br />");
  }
  else
  {
    str += F("><br />");
  }
  str += F("</fieldset>");
  str += F("<span id=\"passwordError\" class=\"error\"></span>");
  str += F("<button type='submit'>{-ACTIONSAVE-}</button></form>");
  str += F("<a href=\"/\" class=\"bt\">{-MENU-}</a>");
  TradAndSend("text/html", str, "", false);
}

void HTTPMgr::handleSetupSave()
{
  if (!ChekifAsAdmin())
  {
    return;
  }

  MainSendDebug("[WWW] Save new setup.");

  if (server.method() == HTTP_POST)
  {
    settings NewConf;
    NewConf.NeedConfig = false;
    strcpy(NewConf.adminPassword, conf.adminPassword);
    strcpy(NewConf.adminUser, conf.adminUser);

    server.arg("ssid").toCharArray(NewConf.ssid, sizeof(NewConf.ssid));
    server.arg("password").toCharArray(NewConf.password, sizeof(NewConf.password));
    server.arg("domoticzIP").toCharArray(NewConf.domoticzIP, sizeof(NewConf.domoticzIP));
    NewConf.domoticzPort = server.arg("domoticzPort").toInt();
    NewConf.domoticzEnergyIdx = server.arg("domoticzEnergyIdx").toInt();
    NewConf.domoticzGasIdx = server.arg("domoticzGasIdx").toInt();
    NewConf.mqtt = (server.arg("mqtt") == "on");
    NewConf.domo = (server.arg("domo") == "on");

    server.arg("mqttIP").toCharArray(NewConf.mqttIP, sizeof(NewConf.mqttIP));
    NewConf.mqttPort = server.arg("mqttPort").toInt();

    server.arg("mqttUser").toCharArray(NewConf.mqttUser, sizeof(NewConf.mqttUser));
    server.arg("mqttPass").toCharArray(NewConf.mqttPass, sizeof(NewConf.mqttPass));
    server.arg("mqttTopic").toCharArray(NewConf.mqttTopic, sizeof(NewConf.mqttTopic));

    NewConf.interval = server.arg("interval").toInt();
    NewConf.InverseHigh_1_2_Tarif = (server.arg("InvTarif") == "on");
    NewConf.telnet = (server.arg("telnet") == "on");
    NewConf.debugToTelnet = (server.arg("debugToTelnet") == "on");
    NewConf.debugToMqtt = (server.arg("debugToMqtt") == "on");

    NewConf.ConfigVersion = SETTINGVERSION;

    MainSendDebug("[HTTP] save in EEPROM !!!");

    RebootPage("Conf-Saved");

    EEPROM.begin(sizeof(struct settings));
    EEPROM.put(0, NewConf);
    EEPROM.commit();

    MainSendDebug("[HTTP] Reboot !!!");
    RequestRestart(1000);
  }
}

void HTTPMgr::RebootPage(String Message)
{
    String str = F("<fieldset><legend>{-ConfH1-}</legend>");
    str += "<p>{-" + Message + "-}</p>";
    str += F("<p>{-ConfReboot-}</p>");
    str += F("<p></p>");
    str += F("<p>{-ConfLedStart-}</p>");
    str += F("<p>{-ConfLedError-}</p>");
    str += GetAnimWait();
    str += F("</fieldset>");

    TradAndSend("text/html", str, "", true);
}

void HTTPMgr::handleP1()
{
  String str = F("<fieldset><legend>{-DATAH1-}</legend>");
  //LastSample
  str += F("<div class=\"row\"><label for='LastSample'>{-DATALastGet-}</label><input type=\"text\" class=\"c6\" id=\"LastSample\"/></div>");
  str += F("<div class=\"row\"><label for='electricityUsedTariff1'>{-DATAFullL-}</label><input type=\"text\" class=\"c6\" id=\"electricityUsedTariff1\"/></div>");
  str += F("<div class=\"row\"><label for='electricityUsedTariff2'>{-DATAFullH-}</label><input type=\"text\" class=\"c6\" id=\"electricityUsedTariff2\"/></div>");
  str += F("<div class=\"row\"><label for='electricityReturnedTariff1'>{-DATAFullProdL-}</label><input type=\"text\" class=\"c6\" id=\"electricityReturnedTariff1\"/></div>");
  str += F("<div class=\"row\"><label for='electricityReturnedTariff2'>{-DATAFullProdH-}</label><input type=\"text\" class=\"c6\" id=\"electricityReturnedTariff2\"/></div>");
  str += F("<div class=\"row\"><label for='actualElectricityPowerDeli'>{-DATACurAmp-}</label><input type=\"text\" class=\"c6\" id=\"actualElectricityPowerDeli\"/></div>");
  str += F("<div class=\"row\"><label for='actualElectricityPowerRet'>{-DATACurProdAmp-}</label><input type=\"text\" class=\"c6\" id=\"actualElectricityPowerRet\"/></div>");
  str += F("<div class=\"row\"><label for='instantaneousVoltageL1'>{-DATAUL1-}</label><input type=\"text\" class=\"c6\" id=\"instantaneousVoltageL1\"/></div>");
  str += F("<div class=\"row\"><label for='instantaneousVoltageL2'>{-DATAUL2-}</label><input type=\"text\" class=\"c6\" id=\"instantaneousVoltageL2\"/></div>");
  str += F("<div class=\"row\"><label for='instantaneousVoltageL3'>{-DATAUL3-}</label><input type=\"text\" class=\"c6\" id=\"instantaneousVoltageL3\"/></div>");
  str += F("<div class=\"row\"><label for='instantaneousCurrentL1'>{-DATAAL1-}</label><input type=\"text\" class=\"c6\" id=\"instantaneousCurrentL1\"/></div>");
  str += F("<div class=\"row\"><label for='instantaneousCurrentL2'>{-DATAAL2-}</label><input type=\"text\" class=\"c6\" id=\"instantaneousCurrentL2\"/></div>");
  str += F("<div class=\"row\"><label for='instantaneousCurrentL3'>{-DATAAL3-}</label><input type=\"text\" class=\"c6\" id=\"instantaneousCurrentL3\"/></div>");
  str += F("<div class=\"row\"><label for='gasReceived5min'>{-DATAGFull-}</label><input type=\"text\" class=\"c6\" id=\"gasReceived5min\"/></div>");
  str += F("</fieldset>");
  str += F("<a href=\"/P1.json\" class=\"bt\">{-SHOWJSON-}</a>");
  str += F("<a href=\"/raw\" class=\"bt\">{-SHOWRAW-}</a>");
  str += F("<a href=\"/\" class=\"bt\">{-MENU-}</a>");
  TradAndSend("text/html", str, "<script type=\"text/javascript\" src=\"P1.js\"></script>", false);
}

void HTTPMgr::handleHelp()
{
  String str = F("<fieldset><legend>{-HLPH1-}</legend>");
  str += F("<p>{-HLPTXT1-}</p>");
  str += F("<p>{-HLPTXT2-}</p>");
  str += F("<p>{-HLPTXT3-}</p>");
  str += F("<p>{-HLPTXT4-}</p>");
  str += F("<p>{-HLPTXT5-}</p>");
  str += F("<p>{-HLPTXT6-}</p>");
  str += F("<p>{-HLPTXT7-}</p>");
  TradAndSend("text/html", str, "", false);
}

void HTTPMgr::handleJSONStatus()
{
  String str;
  JsonDocument doc;

  doc["P1"]["LastSample"] = P1Captor.DataReaded.P1timestamp;
  doc["P1"]["Interval"] = conf.interval;
  doc["P1"]["NextUpdateIn"] = P1Captor.GetnextUpdateTime()-millis();
  if (conf.mqtt)
  {
    doc["MQTT"] = MQTT.IsConnected();
  }

  serializeJson(doc, str);
  
  ActifCache(false);
  server.send(200, "application/json", str);
}

void HTTPMgr::handleJSON()
{
  String str;
  JsonDocument doc;

  doc["LastSample"] = P1Captor.DataReaded.P1timestamp;
  doc["NextUpdateIn"] = P1Captor.GetnextUpdateTime()-millis();
  doc["DataReaded"]["electricityUsedTariff1"] = P1Captor.DataReaded.electricityUsedTariff1.val();
  doc["DataReaded"]["electricityUsedTariff2"] = P1Captor.DataReaded.electricityUsedTariff2.val();
  doc["DataReaded"]["electricityReturnedTariff1"] = P1Captor.DataReaded.electricityReturnedTariff1.val();
  doc["DataReaded"]["electricityReturnedTariff2"] = P1Captor.DataReaded.electricityReturnedTariff2.val();
  doc["DataReaded"]["actualElectricityPowerDeli"] = P1Captor.DataReaded.actualElectricityPowerDeli.val();
  doc["DataReaded"]["actualElectricityPowerRet"] = P1Captor.DataReaded.actualElectricityPowerRet.val();
  doc["DataReaded"]["instantaneousVoltage"]["L1"] = P1Captor.DataReaded.instantaneousVoltageL1.val();
  doc["DataReaded"]["instantaneousVoltage"]["L2"] = P1Captor.DataReaded.instantaneousVoltageL2.val();
  doc["DataReaded"]["instantaneousVoltage"]["L3"] = P1Captor.DataReaded.instantaneousVoltageL3.val();
  doc["DataReaded"]["instantaneousCurrent"]["L1"] = P1Captor.DataReaded.instantaneousCurrentL1.val();
  doc["DataReaded"]["instantaneousCurrent"]["L2"] = P1Captor.DataReaded.instantaneousCurrentL2.val();
  doc["DataReaded"]["instantaneousCurrent"]["L3"] = P1Captor.DataReaded.instantaneousCurrentL3.val();
  doc["DataReaded"]["gasReceived5min"] = P1Captor.DataReaded.gasReceived5min;

  serializeJson(doc, str);

  ActifCache(false);
  server.send(200, "application/json", str);
}

/// @brief Check and ask login to login
/// @return true if logged
bool HTTPMgr::ChekifAsAdmin()
{
  if (strlen(conf.adminPassword) != 0)
  {
    if (!server.authenticate(conf.adminUser, conf.adminPassword))
    {
      server.requestAuthentication();
      return false;
    }
  }
  return true;
}

String HTTPMgr::nettoyerInputText(String inputText)
{
    inputText.replace("'",  "&apos;");
    return inputText;
}

String HTTPMgr::GetAnimWait()
{
  return F("<svg width='100' height='100' viewBox='0 0 100 100' xmlns='http://www.w3.org/2000/svg'><circle cx='50' cy='50' r='40' stroke='#ccc' stroke-width='4' fill='none' /><circle cx='50' cy='10' r='6' fill='#007bff'><animateTransform attributeName='transform' type='rotate' from='0 50 50' to='360 50 50' dur='1s' repeatCount='indefinite' /></circle><circle cx='90' cy='50' r='6' fill='#007bff'><animateTransform attributeName='transform' type='rotate' from='0 50 50' to='360 50 50' dur='2s' repeatCount='indefinite' /></circle><circle cx='50' cy='90' r='6' fill='#007bff'><animateTransform attributeName='transform' type='rotate' from='0 50 50' to='360 50 50' dur='3s' repeatCount='indefinite' /></circle><circle cx='10' cy='50' r='6' fill='#007bff'><animateTransform attributeName='transform' type='rotate' from='0 50 50' to='360 50 50' dur='4s' repeatCount='indefinite' /></circle></svg>");
}

void HTTPMgr::TradAndSend(const char *content_type, String content, String header, bool refresh)
{
  // HEADER
  String str = F("<!DOCTYPE html><html lang='{-HEADERLG-}'><head><link rel=\"icon\" href=\"favicon.svg\">");

  if (refresh)
  {
    str += F("<script>function chk() {fetch('http://' + window.location.hostname).then(response => {if (response.ok) {setTimeout(function () {window.location.href = '/';}, 3000);}}).catch(ex =>{});};setTimeout(setInterval(chk, 3000), 3000);</script>");
  }

  str += F("<meta charset='utf-8'><meta name=\"viewport\" content=\"width=device-width,initial-scale=1,user-scalable=no\"/>");
  str += "<title>" + String(GetClientName()) + "</title>";
  str += F("<script type=\"text/javascript\" src=\"main.js\"></script>");
  str += header;
  str += F("<link rel='stylesheet' type='text/css' href='style.css'></head>");
  str += F("<body><div class=\"container\"><h2>P1 wifi-gateway</h2>");
  str += F("<p class=\"help\"><a href='/Help' target='_blank'>{-HLPH1-}</a>");

  // CONTENT
  str += content;

  // STATUS
  str += F("<div class=\"status-bar\">");
  if (conf.mqtt)
  {
    str += F("<div class=\"item\"><span class=\"indicator\" id=\"MQTT-indicator\"></span><span class=\"text\">MQTT</span></div>");
  }
  str += F("<div class=\"item\"><span class=\"indicator\" id=\"P1-indicator\"></span><span class=\"text\">P1</span></div>");
  str += F("</div>");
  // FOOTER
  str += F("</div>");
  str += F("{-OTAFIRMWARE-} : v");
  str += F(VERSION);
  str += ".";
  str += BUILD_DATE;
  str += F(" | <a href='https://github.com/narfight/P1-wifi-gateway' target='_blank'>Github</a> </body></html>");
  
  Trad.FindAndTranslateAll(str);
  server.send(200, content_type, str);
}
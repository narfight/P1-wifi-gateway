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
  MDNS.begin(HOSTNAME);

  server.on("/style.css", std::bind(&HTTPMgr::handleStyleCSS, this));
  server.on("/", std::bind(&HTTPMgr::handleRoot, this));
  server.on("/setPassword", std::bind(&HTTPMgr::handlePassword, this));
  server.on("/Setup", std::bind(&HTTPMgr::handleSetup, this));
  server.on("/SetupSave", std::bind(&HTTPMgr::handleSetupSave, this));
  server.on("/reset", std::bind(&HTTPMgr::handleFactoryReset, this));
  server.on("/P1", std::bind(&HTTPMgr::handleP1, this));
  server.on("/Help", std::bind(&HTTPMgr::handleHelp, this));
  server.on("/update", HTTP_GET, std::bind(&HTTPMgr::handleUploadForm, this));
  server.on("/update", HTTP_POST, [this]()
  {
    if (!ChekifAsAdmin())
    {
      return;
    }

    if (Update.hasError())
    {
      ReplyOTANOK(Update.getErrorString(), 3);
    }
  }, std::bind(&HTTPMgr::handleUploadFlash, this));

  server.begin();
  MDNS.addService("http", "tcp", WWW_PORT_HTTP);
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
  str += F("<form action='/P1' method='post'><button type='p1' class='bhome'>{-MENUP1-}</button></form>");
  str += F("</fieldset>");
  str += F("<fieldset><legend>{-ConfH1-}</legend>");
  str += F("<form action='/Setup' method='post'><button type='Setup'>{-MENUConf-}</button></form>");
  str += F("<form action='/setPassword' method='post'><button type='Setup'>{-MENUPASSWORD-}</button></form>");
  str += F("<form action='/update' method='GET'><button type='submit'>{-MENUOTA-}</button></form>");
  str += F("<form action='/reset' id=\"frmRst\" method='GET'><button type='button' onclick='ConfRST()'>{-MENURESET-}</button></form>");
  str += F("<script> function ConfRST() { if (confirm(\"{-ASKCONFIRM-}\")) { document.getElementById(\"frmRst\").submit();}}</script></fieldset>");
  TradAndSend("text/html", str, false);
}

void HTTPMgr::ReplyOTAOK()
{
  String str = F("<fieldset><p>{-OTASUCCESS1-}</p><p>{-OTASUCCESS2-}</p><p>{-OTASUCCESS3-}</p><p>{-OTASUCCESS4-}</p><p>{-OTASUCCESS5-}</p>");
  str += GetAnimWait();
  str += F("</fieldset>");
  TradAndSend("text/html", str, true);
}

void HTTPMgr::ReplyOTANOK(const String Error, u_int ref)
{
  MainSendDebugPrintf("[FLASH] Error : %s (%u)", Update.getErrorString(), ref);
  String str = F("<fieldset><p>{-OTANOTSUCCESS-} : <strong>") + Error + " (" + String(ref) + F(")</strong></p><p>{-OTASUCCESS2-}</p><p>{-OTASUCCESS3-}</p><p>{-OTASUCCESS4-}</p><p>{-OTASUCCESS5-}</p>");
  str += GetAnimWait();
  str += F("</fieldset>");
  TradAndSend("text/html", str, true);
  
  Yield_Delay(1000);
  ESP.restart();
}

void HTTPMgr::handleStyleCSS()
{
  MainSendDebug("[HTTP] Request style.css");

  //Gestion du cache sur base de la version du firmware
  String etag = "W/\"" + String(VERSION) + "\"";
  if (server.header("If-None-Match") == etag)
  {
      server.send(304);
      return;
  }

  String str = F("body {text-align: center; font-family: verdana, sans-serif; background: #ffffff;}");
  str += F("h2 {text-align:center;color:#000000;}");
  str += F("div, fieldset, input {padding: 5px; font-size: 1em}");
  str += F("fieldset {background: #ECEAE4;margin-bottom: 20px}");
  str += F("legend {font-weight: bold;}");
  str += F("label {display: inline-block;width:50%;text-align: right;}");
  str += F(".help, .footer {text-align:right;font-size:11px;color:#aaa}");
  str += F("p {margin: 0.5em 0;}");
  str += F("input {width: 240px;box-sizing: border-box; -webkit-box-sizing: border-box; -moz-box-sizing: border-box; background: #ffffff; color: #000000;}");
  str += F("textarea {resize: vertical; width: 98%; height: 318px; padding: 5px; overflow: auto; background: #ffffff; color: #000000;}");
  str += F("button {border: 0; border-radius: 0.3rem; background: #97C1A9; color: #ffffff; line-height: 2.4rem; font-size: 1.2rem; width: 100%; -webkit-transition-duration: 0.4s; transition-duration: 0.4s; cursor: pointer;margin-top: 5px;}");
  str += F("button:hover {background: #0e70a4;}");
  str += F(".bhome {background: #55CBCD;}");
  str += F(".bhome:hover {background: #A2E1DB;}");
  str += F("a {color: #1fa3ec;text-decoration: none;}");
  str += F(".column {float: left;width: 48%;}");
  str += F(".column3 {float: left; width: 31%;}");
  str += F(".row:after {content: \"\";display: table; clear: both;}");
  str += F("input.c6 {text-align:right}");
  str += F("input.c7 {text-align:right; color:#97C1A9}");
  str += F("svg {display: block;margin: auto;}");

  // Cache
  server.sendHeader("ETag", etag);
  server.sendHeader("Cache-Control", "max-age=86400");
  
  // no translate for CSS
  server.send(200, "text/css", str);
}

void HTTPMgr::handleUploadForm()
{
  if (!ChekifAsAdmin())
  {
    return;
  }

  String str = F("<form method='POST' action='' enctype='multipart/form-data'>");
  str += F("<fieldset><legend>{-OTAH1-}</legend>");
  str += F("<p><label for=\"firmware\">{-OTAFIRMWARE-} :</label><input type='file' accept='.bin,.bin.gz' id='firmware' name='firmware'></p>");
  str += F("</fieldset><button type='submit'>{-OTABTUPDATE-}</button></form>");
  str += F("<form action='/' method='POST'><button class='bhome'>{-MENU-}</button></form>");
  TradAndSend("text/html", str, false);
}

void HTTPMgr::handleUploadFlash()
{
  if (ChekifAsAdmin())
  {
    HTTPUpload& upload = server.upload();

    if (upload.status == UPLOAD_FILE_START)
    {
      Update.clearError();
      MainSendDebugPrintf("[FLASH] Upload of '%s'", upload.filename.c_str());
      uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
      MainSendDebugPrintf("[FLASH] Space in memory : %lu", maxSketchSpace);
      if (!Update.begin(maxSketchSpace, U_FLASH))
      {
        ReplyOTANOK(Update.getErrorString(), 0);
      }
    }
    else if (upload.status == UPLOAD_FILE_WRITE)
    {
      MainSendDebug("[FLASH] UPLOAD_FILE_WRITE");
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
      {
        ReplyOTANOK(Update.getErrorString(), 1);
      }
    }
    else if (upload.status == UPLOAD_FILE_END)
    {
      if (Update.end(true)) // true to set the size to the current progress
      {
        MainSendDebug("[FLASH] Update Success, Rebooting...");
        ReplyOTAOK();
        Yield_Delay(1000);
        ESP.restart();
      }
      else
      {
        ReplyOTANOK(Update.getErrorString(), 2);
      }
    }
    else if(upload.status == UPLOAD_FILE_ABORTED)
    {
      Update.end();
      ReplyOTANOK("Update was aborted", 3);
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

  String str = F("<fieldset><legend>{-RF_RESTARTH1-}</legend>");
  str += F("<p>{-RF_RESTTXT-}</p>");
  str += GetAnimWait();
  str += F("</fieldset>");
  TradAndSend("text/html", str, true);

  MainSendDebug("Reset factory and reboot...");

  conf.ConfigVersion = SETTINGVERSIONNULL;

  EEPROM.begin(sizeof(struct settings));
  EEPROM.put(0, conf);
  EEPROM.commit();

  Yield_Delay(1000);
  ESP.reset();
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

  String str = F("<form action=\"/setPassword\" method=\"POST\" onsubmit=\"return Check()\">");
  str += F("<fieldset><legend>{-H1Welcome-}</legend>");
  str += F("<label for=\"adminUser\">{-PSWDLOGIN-} :</label><input type=\"text\" name=\"adminUser\" id=\"adminUser\" maxlength=\"32\" value='");
  str += nettoyerInputText(conf.adminUser);
  str += F("' \"><br />");
  str += F("<label for=\"psd1\">{-PSWD1-} :</label><input type=\"password\" name=\"psd1\" id=\"psd1\" maxlength=\"32\"><br />");
  str += F("<label for=\"psd2\">{-PSWD2-} :</label><input type=\"password\" name=\"psd2\" id=\"psd2\" maxlength=\"32\"><br />");
  str += F("<span id=\"passwordError\" class=\"error\"></span>");
  str += F("</fieldset><button type='submit'>{-ConfSave-}</button></form>");
  str += F("<form action='/' method='POST'><button class='bhome'>{-MENU-}</button></form>");
  TradAndSend("text/html", str, false);
}

void HTTPMgr::handleSetup()
{
  if (!ChekifAsAdmin())
  {
    return;
  }

  String str = F("<form action='/SetupSave' method='POST'>");
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
  str += F("<label for=\"watt\">{-ConfMQTTKW-} :</label><input type='checkbox' name='watt' id='watt' ");
  if (conf.watt)
  {
    str += F(" checked></p>");
  }
  else
  {
    str += F("><br />");
  }
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
  str += F("<label for=\"debugToMqtt\">{-ConfMQTTDBG-} :</label><input type='checkbox' name='debugToMqtt' id='debugToMqtt' ");
  if (conf.debugToMqtt)
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
  str += F("<form action='/' method='POST'><button class='bhome'>{-MENU-}</button></form>");
  TradAndSend("text/html", str, false);
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

    server.arg("adminUser").toCharArray(NewConf.adminUser, sizeof(conf.adminUser));
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
    NewConf.watt = (server.arg("watt") == "on");
    NewConf.telnet = (server.arg("telnet") == "on");
    NewConf.debugToTelnet = (server.arg("debugToTelnet") == "on");
    NewConf.debugToMqtt = (server.arg("debugToMqtt") == "on");

    NewConf.ConfigVersion = SETTINGVERSION;

    MainSendDebug("[HTTP] save in EEPROM !!!");

    String str = F("<fieldset><legend>{-ConfH1-}</legend>");
    str += F("<p>{-Conf-Saved-}</p>");
    str += F("<p>{-ConfReboot-}</p>");
    str += F("<p></p>");
    str += F("<p>{-ConfLedStart-}</p>");
    str += F("<p>{-ConfLedError-}</p>");
    str += GetAnimWait();
    str += F("</fieldset>");

    TradAndSend("text/html", str, true);

    EEPROM.begin(sizeof(struct settings));
    EEPROM.put(0, NewConf);
    EEPROM.commit();

    MainSendDebug("[HTTP] Reboot !!!");
    Yield_Delay(1000);
    ESP.restart();
  }
}

void HTTPMgr::handleP1()
{
  String eenheid, eenheid2, eenheid3;
  if (conf.watt)
  {
    eenheid = " kWh'></div>";
  }
  else
  {
    eenheid = " Wh'></div>";
  }
  
  if (conf.watt)
  {
    eenheid2 = " kW'></div></p>";
  }
  else
  {
    eenheid2 = " W'></div></p>";
  }

  String str = F("<form ><fieldset><legend>{-DATAH1-}</legend>");

  str += "<p><div class='row'><div class='column'><b>{-DATAFullL-}</b><input type='text' class='c6' value='";
  str += P1Captor.DataReaded.electricityUsedTariff1;
  str += eenheid;
  str += "<div class='column' style='text-align:right'><br><b>{-DATATODAY-}</b><input type='text' class='c7' value='";
  str += atof(P1Captor.DataReaded.electricityUsedTariff1); // - atof(Logger.log_data.dayE1);
  str += eenheid;
  str += "</div></p>";

  str += "<p><div class='row'><div class='column'><b>{-DATAFullH-}</b><input type='text' class='c6' value='";
  str += P1Captor.DataReaded.electricityUsedTariff2;
  str += eenheid;
  str += "<div class='column' style='text-align:right'><br><b>{-DATATODAY-}</b><input type='text' class='c7' value='";
  str += atof(P1Captor.DataReaded.electricityUsedTariff2); // - atof(Logger.log_data.dayE2);
  str += eenheid;
  str += "</div></p>";

  str += "<p><div class='row'><div class='column'><b>{-DATAFullProdL-}</b><input type='text' class='c6' value='";
  str += P1Captor.DataReaded.electricityReturnedTariff1;
  str += eenheid;
  str += "<div class='column' style='text-align:right'><br><b>{-DATATODAY-}</b><input type='text' class='c7' value='";
  str += atof(P1Captor.DataReaded.electricityReturnedTariff1); // - atof(Logger.log_data.dayR1);
  str += eenheid;
  str += "</div></p>";

  str += "<p><div class='row'><div class='column'><b>{-DATAFullProdH-}</b><input type='text' class='c6' value='";
  str += P1Captor.DataReaded.electricityReturnedTariff2;
  str += eenheid;
  str += "<div class='column' style='text-align:right'><br><b>{-DATATODAY-}</b><input type='text' class='c7' value='";
  str += atof(P1Captor.DataReaded.electricityReturnedTariff2); // - atof(Logger.log_data.dayR2);
  str += eenheid;
  str += "</div></p>";

  str += "<p><div class='row'><b>{-DATACurAmp-}</b><input type='text' class='c6' value='";
  str += P1Captor.DataReaded.actualElectricityPowerDeli;
  str += eenheid2;

  str += "<p><div class='row'><b>{-DATACurProdAmp-}</b><input type='text' class='c6' value='";
  str += P1Captor.DataReaded.actualElectricityPowerRet;
  str += eenheid2;

  str += "<p><div class='row'><div class='column3'><b>{-DATAUL1-}</b><input type='text' class='c6' value='";
  str += P1Captor.DataReaded.instantaneousVoltageL1;
  str += " V'></div>";
  str += "<div class='column3' style='text-align:right'><b>{-DATAUL2-}</b><input type='text' class='c7' value='";
  str += P1Captor.DataReaded.instantaneousVoltageL2;
  str += " V'></div>";
  str += "<div class='column3' style='text-align:right'><b>{-DATAUL3-}</b><input type='text' class='c7' value='";
  str += P1Captor.DataReaded.instantaneousVoltageL2;
  str += " V'></div></div></p>";

  str += "<p><div class='row'><div class='column3'><b>{-DATAAL1-}</b><input type='text' class='c6' value='";
  str += P1Captor.DataReaded.instantaneousCurrentL1;
  str += " A'></div>";
  str += "<div class='column3' style='text-align:right'><b>{-DATAAL2-}</b><input type='text' class='c7' value='";
  str += P1Captor.DataReaded.instantaneousCurrentL2;
  str += " A'></div>";
  str += "<div class='column3' style='text-align:right'><b>{-DATAAL3-}</b><input type='text' class='c7' value='";
  str += P1Captor.DataReaded.instantaneousCurrentL3;
  str += " A'></div></div></p>";
  /*

    str += F("<p><b>Voltage dips</b><input type='text' style='text-align:right' value='");
    str += numberVoltageSagsL1;
    str += F("'></p>");
    str += F("<p><b>Voltage pieken</b><input type='text' style='text-align:right' value='");
    str += numberVoltageSwellsL1;
    str += F("'></p>");
    str += F("<p><b>Stroomonderbrekingen</b><input type='text' style='text-align:right' value='");
    str += numberPowerFailuresAny;
    str += F("'></p>");
    */
  str += "<p><div class='row'><div class='column'><b>{-DATAGFull-}</b><input type='text' class='c6' value='";
  str += P1Captor.DataReaded.gasReceived5min;
  str += F(" m3'></div>");
  str += "<div class='column' style='text-align:right'><b>{-DATATODAY-}</b><input type='text' class='c7' value='";
  str += atof(P1Captor.DataReaded.gasReceived5min); // - atof(Logger.log_data.dayG);
  str += " m3'></div></div></p>";
  str += F("</fieldset></form>");
  str += F("<form action='/' method='POST'><button class='bhome'>{-MENU-}</button></form>");
  TradAndSend("text/html", str, false);
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
  TradAndSend("text/html", str, false);
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

void HTTPMgr::TradAndSend(const char *content_type, String content, bool refresh)
{
  // HEADER
  String str = F("<!DOCTYPE html><html lang='{-HEADERLG-}'><head>");

  if (refresh)
  {
    str += F("<script>function chk() {fetch('http://' + window.location.hostname).then(response => {if (response.ok) {setTimeout(function () {window.location.href = '/';}, 3000);}}).catch(ex =>{});};setTimeout(setInterval(chk, 3000), 3000);</script>");
  }

  str += F("<meta charset='utf-8'><meta name=\"viewport\" content=\"width=device-width,initial-scale=1,user-scalable=no\"/>");
  str += F("<title>P1 wifi-gateway</title>");
  str += F("<link rel='stylesheet' type='text/css' href='style.css'></head>");
  str += F("<body><div style='text-align:left;display:inline-block;color:#000000;width:600px;'><h2>P1 wifi-gateway</h2>");
  str += F("<p class=\"help\"><a href='/Help' target='_blank'>{-HLPH1-}</a>");

  // CONTENT
  str += content;

  // FOOTER
  str += F("<div class=\"footer\">");
  if (conf.mqtt)
  {
    if (MQTT.IsConnected())
    {
      str += F("MQTT link: Connected ");
    }
    else
    {
      str += F("MQTT link: Not connected ");
    }
  }
  str += F("{-OTAFIRMWARE-} : ");
  str += F(VERSION);
  str += F("<br><a href='https://github.com/narfight/P1-wifi-gateway' target='_blank'>Github</a>");
  /*if (refresh)
  {
    str += F("<script>setTimeout(setInterval(chk, 2000), 4000);</script>");
  }*/
  str += F("</div></div></body></html>");
  
  Trad.FindAndTranslateAll(str);
  server.send(200, content_type, str);
}
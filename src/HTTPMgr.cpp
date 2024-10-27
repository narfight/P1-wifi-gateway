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
  //MDNS.begin(GetClientName());
  //header files
  server.on("/style.css", std::bind(&HTTPMgr::handleStyleCSS, this));
  server.on("/favicon.svg", std::bind(&HTTPMgr::handleFavicon, this));
  server.on("/main.js", std::bind(&HTTPMgr::handleMainJS, this));
  
  //extra for /P1 page for refresh
  server.on("/P1.json", std::bind(&HTTPMgr::handleJSON, this));
  server.on("/P1.js", std::bind(&HTTPMgr::handleP1Js, this));

  server.on("/Log24H.js", std::bind(&HTTPMgr::handleGraph24JS, this));
  server.on("/Log24H", std::bind(&HTTPMgr::handleGraph24, this));
  
  //for the footer
  server.on("/status.json", std::bind(&HTTPMgr::handleJSONStatus, this));

  server.on("/file", std::bind(&HTTPMgr::handleFile, this));

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
      ReplyOTA(false, UpdateMsg.c_str(), UpdateErrorCode);
      UpdateResultFailed = false;
      UpdateMsg = "";
    }
    else
    {
      ReplyOTA(true, "", 0);
    }
  }, std::bind(&HTTPMgr::handleUploadFlash, this));

  server.begin();
  //MDNS.addService("http", "tcp", WWW_PORT_HTTP);
}

bool HTTPMgr::ActifCache(bool enabled)
{
  if (enabled)
  {
    //Gestion du cache sur base de la version du firmware
    char etag[15];
    snprintf(etag, sizeof(etag), "W/\"%d\"", BUILD_DATE);
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

  // Utiliser directement une chaîne PROGMEM complète plutôt que de multiples concaténations
  static char template_html[] PROGMEM = R"(
    <fieldset><legend>{-H1DATA-}</legend>
    <a href="/P1" class="bt">{-MENUP1-}</a>
    <a href="/Log24H" class="bt">{-MENUGraph24-}</a>
    </fieldset>
    <fieldset><legend>{-ConfH1-}</legend>
    <a href="/Setup" class="bt">{-MENUConf-}</a>
    <a href="/setPassword" class="bt">{-MENUPASSWORD-}</a>
    <a href="/update" class="bt">{-MENUOTA-}</a>
    <a href="/reboot" class="bt bwarning">{-MENUREBOOT-}</a>
    <a href="/reset" class="bt bwarning">{-MENURESET-}</a>
    </fieldset>)";

  TradAndSend("text/html", template_html, "", false);
}

void HTTPMgr::handleFile()
{
  if (!server.hasArg("name"))
  {
    server.send(400, "text/plain", "Missing name parameter");
    return;
  }

  if (!LittleFS.begin())
  {
    server.send(500, "text/plain");
    return;
  }

  if (LittleFS.exists(server.arg("name")))
  {
    File file = LittleFS.open(server.arg("name"), "r");
    if (!file)
    {
      server.send(404, "text/plain", "File not found");
      return;
    }

    const size_t BUFFER_SIZE = 512;  // Ajustez selon votre mémoire disponible
    char buffer[BUFFER_SIZE];
    size_t totalSize = file.size();

    // Configurer l'en-tête pour le streaming
    server.setContentLength(totalSize);
    server.send(200, "application/json", "");

    // Envoyer le fichier par morceaux
    while (file.available())
    {
      size_t bytesRead = file.readBytes(buffer, BUFFER_SIZE);
      if (bytesRead > 0)
      {
        server.client().write(buffer, bytesRead);
      }
    }

    file.close();
  }
  else
  {
    server.send(404, "text/plain");
  }
}

void HTTPMgr::ReplyOTA(bool success, const char* error, u_int ref)
{
  if (success)
  {
    MainSendDebug("[FLASH] Ok, reboot now");
  }
  else
  {
    MainSendDebugPrintf("[FLASH] Error : %s (%u)", error, ref);
  }

  static const char template_html[] PROGMEM = R"(
<fieldset><p>{-OTANSUCCESS%sOK-} : <strong>%s (%u)</strong></p>
<p>{-OTASUCCESS1-}</p>
<p>{-OTASUCCESS2-}</p>
<p>{-OTASUCCESS3-}</p>
<p>{-OTASUCCESS4-}</p>
%s</fieldset>)";

  const char* animation = GetAnimWait();  // Supposons que GetAnimWait retourne un char*
  snprintf_P(HTMLBufferContent, sizeof(HTMLBufferContent), template_html, (success)? "" : "N", error, ref, animation);

  TradAndSend("text/html", HTMLBufferContent, "", true);
  RequestRestart(3000);
}

void HTTPMgr::handleRAW()
{
  server.send(200, "Text/plain", P1Captor.datagram);
}

void HTTPMgr::handleP1Js()
{
  if (ActifCache(true)) return;
  static const char template_html[] PROGMEM = R"(async function updateValues(){try{let e=await fetch("P1.json"),a=await e.json();document.getElementById("LastSample").value=parseDateTime(a.LastSample).toLocaleString(),document.getElementById("T1").value=a.P1.T1+" kWh",document.getElementById("T2").value=a.P1.T2+" kWh",document.getElementById("RT1").value=a.P1.RT1+" kWh",document.getElementById("RT2").value=a.P1.RT2+" kWh",document.getElementById("TA").value=a.P1.TA+" kWh",document.getElementById("RTA").value=a.P1.RTA+" kWh",document.getElementById("VL1").value=a.P1.V.L1+" V",document.getElementById("VL2").value=a.P1.V.L2+" V",document.getElementById("VL3").value=a.P1.V.L3+" V",document.getElementById("AL1").value=a.P1.A.L1+" A",document.getElementById("AL2").value=a.P1.A.L2+" A",document.getElementById("AL3").value=a.P1.A.L3+" A",document.getElementById("gasReceived5min").value=a.P1.gasReceived5min+" m3"}catch(t){console.error("Error on update :",t)}}setInterval(updateValues,1e4),window.onload=updateValues;)";
  server.send(200, "application/javascript", template_html);
}

void HTTPMgr::handleStyleCSS()
{
  if (ActifCache(true)) return;

  static const char css[] PROGMEM = R"(
body {font-family: Verdana, sans-serif;background-color: #f9f9f9;margin: 0;padding: 20px;text-align: center;}
.container {max-width: 600px;margin: 0 auto;background: #ffffff;border-radius: 8px;box-shadow: 0 2px 10px rgba(0, 0, 0, 0.1);padding: 20px 20px 0px 20px;}
h2 {text-align:center;color:#000000;}
fieldset {border: 1px solid #ddd;border-radius: 8px;padding: 10px;margin-bottom: 20px;}
fieldset input {width: 30%;padding: 5px;border: 1px solid #ddd;border-radius: 4px;font-size: 1em;box-shadow: inset 0 1px 3px rgba(0, 0, 0, 0.1)}
legend {font-weight: bold;padding: 0 10px;font-size: 1.2em}
label {display: inline-block;text-align: right;width: 60%;text-align: right;margin-right: 10px;margin-bottom: 12px;font-weight: normal}
.help, .footer {text-align:right;font-size:11px;color:#aaa}
p {margin: 0.5em 0;}
button, .bt {display: inline-block;text-align: center;text-decoration: none;border: 0;border-radius: 0.3rem;background: #97C1A9;color: #ffffff;line-height: 2.4rem;font-size: 1.2rem;width: 100%;-webkit-transition-duration: 0.4s;transition-duration: 0.4s;cursor: pointer;margin-top: 5px;}
button:hover, .bt:hover {background: #0e70a4;}
.bt[href="/"], .bt[href="/P1"]{background: #55CBCD;}
.bt[href="/"]:hover, .bt[href="/P1"]:hover {background: #A2E1DB;}
.bwarning {background: #E74C3C;}
.bwarning:hover {background: #C0392B;}
a {color: #1fa3ec;text-decoration: none;}
.row:after {content: "";display: table; clear: both;}
svg {display: block;margin: auto;}
.status-bar {display: flex;justify-content: flex-end;margin-top: 10px;padding: 10px;border-top: 1px solid #ddd;}
.status-bar .indicator {width: 10px;height: 10px;border-radius: 50%;background-color: green;margin-right: 5px;display: inline-block;}
.error {background-color: red !important;}
.status-bar .text {margin-left: 5px;}
.status-bar .item {display: flex;align-items: center;margin-bottom: 5px;padding-left: 10px}
)";

  server.send(200, "text/css", css);
}

void HTTPMgr::handleMainJS()
{
  if (ActifCache(true)) return;

  static char js[] PROGMEM = R"(function parseDateTime(t){return new Date("20"+t.substring(0,2),t.substring(2,4)-1,t.substring(4,6),t.substring(6,8),t.substring(8,10),t.substring(10,12))}async function updateStatus(){try{let e=await fetch("status.json"),s=await e.json();const r=document.getElementById("MQTT-indicator");null!=r&&(1==s.MQTT?r.classList.remove("error"):r.classList.add("error"));const n=document.getElementById("P1-indicator");if(""!=s.P1.LastSample){var t=parseDateTime(s.P1.LastSample);Date.now().set;t.setSeconds(t.getSeconds()+3*s.P1.Interval),t<Date.now()?n.classList.add("error"):n.classList.remove("error")}else n.classList.add("error")}catch(t){console.error("Error on update status:",t)}}window.onload=function(){updateStatus();document.querySelectorAll(".bwarning").forEach((t=>{t.addEventListener("click",(function(t){confirm("{-ASKCONFIRM-}")||t.preventDefault()}))})),setInterval(updateStatus,1e4)};)";

  char* translatedText = Trad.FindAndTranslateAll(js);
  server.send(200, "application/javascript", translatedText);
  free(translatedText);
}

void HTTPMgr::handleGraph24JS()
{
  if (ActifCache(true)) return;

  static char js[] PROGMEM = R"(function formatDate(l){return`20${l.slice(0,2)}-${l.slice(2,4)}-${l.slice(4,6)} ${l.slice(6,8)}:${l.slice(8,10)}:${l.slice(10)}`}google.charts.load("current",{packages:["corechart","bar"]}),google.charts.setOnLoadCallback(()=>{fetch("/file?name=/Last24H.json").then(l=>l.json()).then(l=>{var e=new google.visualization.DataTable;e.addColumn("string","DateTime"),e.addColumn("number","T1"),e.addColumn("number","T2"),e.addColumn("number","R1"),e.addColumn("number","R2");let a={T1:null,T2:null,R1:null,R2:null};l.forEach(l=>{let n={T1:null,T2:null,R1:null,R2:null};null!==a.T1&&(n.T1=l.T1-a.T1,n.T2=l.T2-a.T2,n.R1=l.R1-a.R1,n.R2=l.R2-a.R2),a={T1:l.T1,T2:l.T2,R1:l.R1,R2:l.R2},e.addRow([formatDate(l.DateTime),n.T1,n.T2,n.R1,n.R2])}),new google.visualization.LineChart(document.getElementById("chart_div")).draw(e,{hAxis:{title:"Date/Heure"},vAxis:{title:"kWh",format:"# kWh"},legend:"bottom", chartArea: {width:'90%'}})})});)";

  server.send(200, "application/javascript", js);
}

void HTTPMgr::handleGraph24()
{
  if (!ChekifAsAdmin())
  {
    return;
  }

 static char html[] PROGMEM = R"(<fieldset><legend>{-MENUGraph24-}</legend></h1>
    <div id="chart_div" style="width: 100%"></div></fieldset><a href="/" class="bt">{-MENU-}</a>)";

  TradAndSend("text/html", html, "<script type=\"text/javascript\" src=\"https://www.gstatic.com/charts/loader.js\"></script><script type=\"text/javascript\" src=\"Log24H.js\"></script>", false);
}

void HTTPMgr::handleFavicon()
{
  if (ActifCache(true)) return;

  static const char fav[] PROGMEM = R"(<?xml version="1.0" encoding="UTF-8"?><svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" version="1.1" viewBox="0 0 24 24"><path d="M4,4H20A2,2 0 0,1 22,6V18A2,2 0 0,1 20,20H4A2,2 0 0,1 2,18V6A2,2 0 0,1 4,4M4,6V18H11V6H4M20,18V6H18.76C19,6.54 18.95,7.07 18.95,7.13C18.88,7.8 18.41,8.5 18.24,8.75L15.91,11.3L19.23,11.28L19.24,12.5L14.04,12.47L14,11.47C14,11.47 17.05,8.24 17.2,7.95C17.34,7.67 17.91,6 16.5,6C15.27,6.05 15.41,7.3 15.41,7.3L13.87,7.31C13.87,7.31 13.88,6.65 14.25,6H13V18H15.58L15.57,17.14L16.54,17.13C16.54,17.13 17.45,16.97 17.46,16.08C17.5,15.08 16.65,15.08 16.5,15.08C16.37,15.08 15.43,15.13 15.43,15.95H13.91C13.91,15.95 13.95,13.89 16.5,13.89C19.1,13.89 18.96,15.91 18.96,15.91C18.96,15.91 19,17.16 17.85,17.63L18.37,18H20M8.92,16H7.42V10.2L5.62,10.76V9.53L8.76,8.41H8.92V16Z"/></svg>)";
  
  server.send(200, "image/svg+xml", fav);
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

 static char html[] PROGMEM = R"(
<form method='post' action='' enctype='multipart/form-data'>
  <fieldset><legend>{-OTAH1-}</legend>
  <p><label for="firmware">{-OTAFIRMWARE-} :</label><input type='file' accept='.bin,.bin.gz' id='firmware' name='firmware' required></p>
  </fieldset><button class="bt bwarning" type='submit'>{-OTABTUPDATE-}</button></form>
  <a href="/" class="bt">{-MENU-}</a>)";

  TradAndSend("text/html", html, "", false);
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
      MainSendDebugPrintf("[FLASH] Upload of '%s' (%lu octet - free %lu)", upload.filename.c_str(), upload.contentLength, maxSketchSpace);

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
      MainSendDebug("[HTTP] New password");
      EEPROM.begin(sizeof(struct settings));
      EEPROM.put(0, conf);
      EEPROM.commit();

      // Move to full setup !
      server.sendHeader("Location", "/");
      server.send(302, "text/plain", "Redirecting");
      return;
    }
  }
  
static const char template_html[] PROGMEM = R"(
<form action="/setPassword" method="post" onsubmit='return Check()'>
<fieldset><legend>{-H1Welcome-}</legend>
<label for="adminUser">{-PSWDLOGIN-} :</label><input type="text" name="adminUser" id="adminUser" maxlength="32" value="%s" /><br />
<label for="psd1">{-PSWD1-} :</label><input type="password" name="psd1" id="psd1" maxlength="32"><br />
<label for="psd2">{-PSWD2-} :</label><input type="password" name="psd2" id="psd2" maxlength="32"><br />
<span id="passwordError" class="error"></span>
</fieldset><button type="submit">{-ConfSave-}</button></form>
<a href="/" class="bt">{-MENU-}</a>
)";

  snprintf_P(HTMLBufferContent, sizeof(HTMLBufferContent), template_html,
    nettoyerInputText(conf.adminUser, 33)
  );
  //
  TradAndSend("text/html", HTMLBufferContent, "", false);
}

void HTTPMgr::handleSetup()
{
  if (!ChekifAsAdmin())
  {
    return;
  }

static const char template_html[] PROGMEM = R"(
<form action="/SetupSave" method="post">
<fieldset><legend>{-ConfWIFIH2-}</legend>
<label for="ssid">{-ConfSSID-} :</label><input type="text" name="ssid" id="ssid" maxlength="32" value="%s"><br />
<label for="password">{-ConfWIFIPWD-} :</label><input type="password" maxlength="64" name="password" id="password" value="%s"><br />
</fieldset>
<fieldset><legend>{-ConfDMTZH2-}</legend>
<label for="domo">{-ConfDMTZBool-} :</label><input type="checkbox" name="domo" id="domo" %s><br />
<label for="domoticzIP">{-ConfDMTZIP-} :</label><input type="text" name="domoticzIP" id="domoticzIP" maxlength="29" value="%s"><br />
<label for="domoticzPort">{-ConfDMTZPORT-} :</label><input type="number" min="1" max="65535" id="domoticzPort" name="domoticzPort" value="%u"><br />
<label for="domoticzGasIdx">{-ConfDMTZGIdx-} :</label><input type="number" min="1" id="domoticzGasIdx" name="domoticzGasIdx" value="%u"><br />
<label for="domoticzEnergyIdx">{-ConfDMTZEIdx-} :</label><input type="number" min="1" id="domoticzEnergyIdx" name="domoticzEnergyIdx" value="%u">
</fieldset>
<fieldset><legend>{-ConfMQTTH2-}</legend>
<label for="mqtt">{-ConfMQTTBool-} :</label><input type="checkbox" name="mqtt" id="mqtt" %s><br />
<label for="mqttIP">{-ConfMQTTIP-} :</label><input type="text" id="mqttIP" name="mqttIP" maxlength="29" value="%s"><br />
<label for="mqttPort">{-ConfMQTTPORT-} :</label><input type="number" min="1" max="65535" id="mqttPort" name="mqttPort" value="%u"><br />
<label for="mqttUser">{-ConfMQTTUsr-} :</label><input type="text" id="mqttUser" name="mqttUser" maxlength="31" value="%s"><br />
<label for="mqttPass">{-ConfMQTTPSW-} :</label><input type="password" id="mqttPass" name="mqttPass" maxlength="31" value="%s"><br />
<label for="mqttTopic">{-ConfMQTTRoot-} :</label><input type="text" id="mqttTopic" name="mqttTopic" maxlength="49" value="%s"><br />
<label for="interval">{-ConfMQTTIntr-} :</label><input type="number" min="10" id="interval" name="interval" value="%u"><br />
<label for="InvTarif">{-ConfPERMUTTARIF-} :</label><input type="checkbox" name="InvTarif" id="InvTarif" %s><br />
<label for="debugToMqtt">{-ConfMQTTDBG-} :</label><input type="checkbox" name="debugToMqtt" id="debugToMqtt" %s><br />
</fieldset>
<fieldset><legend>{-ConfTLNETH2-}</legend>
<label for="telnet">{-ConfTLNETBool-} :</label><input type="checkbox" name="telnet" id="telnet" %s><br />
<label for="debugToTelnet">{-ConfTLNETDBG-} :</label><input type="checkbox" name="debugToTelnet" id="debugToTelnet" %s><br />
</fieldset>
<span id="passwordError" class="error"></span>
<button type="submit">{-ACTIONSAVE-}</button></form>
<a href="/" class="bt">{-MENU-}</a>
)";

  snprintf_P(HTMLBufferContent, sizeof(HTMLBufferContent), template_html,
    nettoyerInputText(conf.ssid, 33),
    nettoyerInputText(conf.password, 65),
    (conf.domo)? "checked" : "",
    nettoyerInputText(conf.domoticzIP, 30),
    conf.domoticzPort,
    conf.domoticzGasIdx,
    conf.domoticzEnergyIdx,
    (conf.mqtt)? "checked" : "",
    nettoyerInputText(conf.mqttIP, 30),
    conf.mqttPort,
    nettoyerInputText(conf.mqttUser, 32),
    nettoyerInputText(conf.mqttPass, 32),
    nettoyerInputText(conf.mqttTopic, 50),
    conf.interval,
    (conf.InverseHigh_1_2_Tarif)? "checked" : "",
    (conf.debugToMqtt)? "checked" : "",
    (conf.telnet)? "checked" : "",
    (conf.debugToTelnet)? "checked" : ""
  );

  TradAndSend("text/html", HTMLBufferContent, "", false);
}

void HTTPMgr::handleSetupSave()
{
  if (!ChekifAsAdmin())
  {
    return;
  }

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

    RebootPage("Conf-Saved");

    EEPROM.begin(sizeof(struct settings));
    EEPROM.put(0, NewConf);
    EEPROM.commit();

    RequestRestart(1000);
  }
}

void HTTPMgr::RebootPage(const char *Message)
{
  static const char template_html[] PROGMEM = R"(
<fieldset><legend>{-ConfH1-}</legend>
<p>{-%s-}</p>
<p>{-ConfReboot-}</p>
<p></p>
<p>{-ConfLedStart-}</p>
<p>{-ConfLedError-}</p>
%s
</fieldset>
)";
  
  snprintf_P(HTMLBufferContent, sizeof(HTMLBufferContent), template_html, Message, GetAnimWait());

  TradAndSend("text/html", HTMLBufferContent, "", true);
}

void HTTPMgr::handleP1()
{
  static char template_html[] PROGMEM = R"(
<fieldset><legend>{-DATAH1-}</legend>
<div class="row"><label for="LastSample">{-DATALastGet-}</label><input type="text" class="c6" id="LastSample"/></div>
<div class="row"><label for="T1">{-DATAFullL-}</label><input type="text" class="c6" id="T1"/></div>
<div class="row"><label for="T2">{-DATAFullH-}</label><input type="text" class="c6" id="T2"/></div>
<div class="row"><label for="RT1">{-DATAFullProdL-}</label><input type="text" class="c6" id="RT1"/></div>
<div class="row"><label for="RT2">{-DATAFullProdH-}</label><input type="text" class="c6" id="RT2"/></div>
<div class="row"><label for="TA">{-DATACurAmp-}</label><input type="text" class="c6" id="TA"/></div>
<div class="row"><label for="RTA">{-DATACurProdAmp-}</label><input type="text" class="c6" id="RTA"/></div>
<div class="row"><label for="VL1">{-DATAUL1-}</label><input type="text" class="c6" id="VL1"/></div>
<div class="row"><label for="VL2">{-DATAUL2-}</label><input type="text" class="c6" id="VL2"/></div>
<div class="row"><label for="VL3">{-DATAUL3-}</label><input type="text" class="c6" id="VL3"/></div>
<div class="row"><label for="AL1">{-DATAAL1-}</label><input type="text" class="c6" id="AL1"/></div>
<div class="row"><label for="AL2">{-DATAAL2-}</label><input type="text" class="c6" id="AL2"/></div>
<div class="row"><label for="AL3">{-DATAAL3-}</label><input type="text" class="c6" id="AL3"/></div>
<div class="row"><label for="gasReceived5min">{-DATAGFull-}</label><input type="text" class="c6" id="gasReceived5min"/></div>
</fieldset>
<a href="/P1.json" class="bt">{-SHOWJSON-}</a>
<a href="/raw" class="bt">{-SHOWRAW-}</a>
<a href="/" class="bt">{-MENU-}</a>
)";
  TradAndSend("text/html", template_html, "<script type=\"text/javascript\" src=\"P1.js\"></script>", false);
}

void HTTPMgr::handleHelp()
{
  static char template_html[] PROGMEM = R"(
<fieldset><legend>{-HLPH1-}</legend>
<p>{-HLPTXT1-}</p>
<p>{-HLPTXT2-}</p>
<p>{-HLPTXT3-}</p>
<p>{-HLPTXT4-}</p>
<p>{-HLPTXT5-}</p>
<p>{-HLPTXT6-}</p>
<p>{-HLPTXT7-}</p>
  )";
  TradAndSend("text/html", template_html, "", false);
}

void HTTPMgr::handleJSONStatus()
{
  char out[90];
  JsonDocument doc;

  doc["P1"]["LastSample"] = P1Captor.DataReaded.P1timestamp;
  doc["P1"]["Interval"] = conf.interval;
  doc["P1"]["NextUpdateIn"] = P1Captor.GetnextUpdateTime()-millis();
  if (conf.mqtt)
  {
    doc["MQTT"] = MQTT.IsConnected();
  }

  serializeJson(doc, out);
  
  ActifCache(false);
  server.send(200, "application/json", out);
}

void HTTPMgr::handleJSON()
{
  char str[1000];
  JsonDocument doc;
  doc["LastSample"] = P1Captor.DataReaded.P1timestamp;
  doc["NextUpdateIn"] = P1Captor.GetnextUpdateTime()-millis();
  doc["P1"]["T1"] = P1Captor.DataReaded.electricityUsedTariff1.val();
  doc["P1"]["T2"] = P1Captor.DataReaded.electricityUsedTariff2.val();
  doc["P1"]["RT1"] = P1Captor.DataReaded.electricityReturnedTariff1.val();
  doc["P1"]["RT2"] = P1Captor.DataReaded.electricityReturnedTariff2.val();
  doc["P1"]["TA"] = P1Captor.DataReaded.actualElectricityPowerDeli.val();
  doc["P1"]["RTA"] = P1Captor.DataReaded.actualElectricityPowerRet.val();
  doc["P1"]["V"]["L1"] = P1Captor.DataReaded.instantaneousVoltageL1.val();
  doc["P1"]["V"]["L2"] = P1Captor.DataReaded.instantaneousVoltageL2.val();
  doc["P1"]["V"]["L3"] = P1Captor.DataReaded.instantaneousVoltageL3.val();
  doc["P1"]["A"]["L1"] = P1Captor.DataReaded.instantaneousCurrentL1.val();
  doc["P1"]["A"]["L2"] = P1Captor.DataReaded.instantaneousCurrentL2.val();
  doc["P1"]["A"]["L3"] = P1Captor.DataReaded.instantaneousCurrentL3.val();
  doc["P1"]["gasReceived5min"] = P1Captor.DataReaded.gasReceived5min;

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

char* HTTPMgr::nettoyerInputText(const char* inputText, size_t maxLen)
{
  char* result = (char*)malloc(maxLen + 1); // +1 pour le caractère de fin de chaîne
  if (result == NULL)
  {
    // Gestion d'erreur : allocation mémoire échouée
    return NULL;
  }

  strcpy(result, inputText);

  char* found = result;
  while ((found = strchr(found, '\'')) != NULL)
  {
    // Décalage de tous les caractères après l'apostrophe
    memmove(found + 5, found + 1, strlen(found + 1) + 1); // +1 pour le caractère de fin de chaîne
    // Copie de "&apos;"
    strcpy(found, "&apos;");
    found += 5; // Passer à la prochaine position
  }

  return result;
}

const char* HTTPMgr::GetAnimWait()
{
    static const char anim_wait[] PROGMEM = R"(
<svg width="100" height="100" viewBox="0 0 100 100" xmlns="http://www.w3.org/2000/svg">
    <circle cx="50" cy="50" r="40" stroke="#ccc" stroke-width="4" fill="none" />
    <circle cx="50" cy="10" r="6" fill="#007bff"><animateTransform attributeName="transform" type="rotate" from="0 50 50" to="360 50 50" dur="1s" repeatCount="indefinite" /></circle>
    <circle cx="90" cy="50" r="6" fill="#007bff"><animateTransform attributeName="transform" type="rotate" from="0 50 50" to="360 50 50" dur="2s" repeatCount="indefinite" /></circle>
    <circle cx="50" cy="90" r="6" fill="#007bff"><animateTransform attributeName="transform" type="rotate" from="0 50 50" to="360 50 50" dur="3s" repeatCount="indefinite" /></circle>
    <circle cx="10" cy="50" r="6" fill="#007bff"><animateTransform attributeName="transform" type="rotate" from="0 50 50" to="360 50 50" dur="4s" repeatCount="indefinite" /></circle>
</svg>)";

    return anim_wait;
}

void HTTPMgr::TradAndSend(const char *content_type, char *content, const char *header, bool refresh)
{
  char buffer[1500];  // Buffer pour header et footer
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  static const char template_html_header[] PROGMEM = R"(
<!DOCTYPE html>
<html lang="{-HEADERLG-}">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no">
<link rel="icon" href="favicon.svg">
<link rel="stylesheet" type="text/css" href="style.css">
<script type="text/javascript" src="main.js"></script>
<title>%s</title>
%s
%s
</head>
<body><div class="container"><h2>P1 wifi-gateway</h2>
<p class="help"><a href="/Help" target="_blank">{-HLPH1-}</a></p>)";

  static const char template_html_footer[] PROGMEM = R"(
<div class="status-bar">
<div class="item"><span class="indicator" id="MQTT-indicator"></span><span class="text">MQTT</span></div>
<div class="item"><span class="indicator" id="P1-indicator"></span><span class="text">P1</span></div>
</div>
{-OTAFIRMWARE-} : v%s.%d  | <a href="https://github.com/narfight/P1-wifi-gateway" target="_blank">Github</a></div></body></html>
)";
  snprintf_P(buffer, sizeof(buffer), template_html_header,
    GetClientName(),
    header,
    (refresh)? "<script>function chk() {fetch('http://' + window.location.hostname).then(response => {if (response.ok) {setTimeout(function () {window.location.href = '/';}, 6000);}}).catch(ex =>{});};setTimeout(setInterval(chk, 3000), 3000);</script>" : ""
  );
  char* translatedText = Trad.FindAndTranslateAll(buffer);
  server.sendContent(translatedText);
  free(translatedText);

  //content
  translatedText = Trad.FindAndTranslateAll(content);
  server.sendContent(translatedText);
  free(translatedText);

  snprintf_P(buffer, sizeof(buffer), template_html_footer,
    VERSION,
    BUILD_DATE
  );

  translatedText = Trad.FindAndTranslateAll(buffer);
  server.sendContent(translatedText);
  free(translatedText);
}
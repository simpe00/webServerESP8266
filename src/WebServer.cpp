// Webserver example

#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <FS.h>       // File System for Web Server Files
#include <LittleFS.h> // This file system is used.

#include <secrets.h> // add WLAN Credentials in here.
#include "builtinfiles.h" // The text of builtin files are in this header file


// #define UNUSED __attribute__((unused)) // mark parameters not used in example
#define TRACE(...) Serial.println(__VA_ARGS__) // TRACE output simplified, can be deactivated here

#define HOSTNAME "webserver"
#define TIMEZONE "CET-1CEST,M3.5.0,M10.5.0/3"

// need a WebServer for http access on port 80.
ESP8266WebServer server(80);

bool stateButton1;
bool stateButton2;


// ===== Simple functions used to answer simple GET requests =====

void handleRedirect() {
  String url = "/index.htm";

  if (!LittleFS.exists(url)) {
    url = "/$update.htm";
  }

  server.sendHeader("Location", url, true);
  server.send(302);
}


void getBackToButtonPageWithQuery() {
  String stateTemp = (stateButton1) ? "1" : "0";
  String url = "/button.htm?B1=" + stateTemp;
  stateTemp = (stateButton2) ? "1" : "0";
  url = url + "&B2="+ stateTemp;

  server.sendHeader("Location", url, true);
  server.send(302);
}


void handleListFiles() {
  Dir dir = LittleFS.openDir("/");
  String result;

  result += "[\n";
  while (dir.next()) {
    if (result.length() > 4) {
      result += ",";
    }
    result += "  {";
    result += " \"name\": \"" + dir.fileName() + "\", ";
    result += " \"size\": " + String(dir.fileSize()) + ", ";
    result += " \"time\": " + String(dir.fileTime());
    result += " }\n";
  }

  result += "]";
  server.sendHeader("Cache-Control", "no-cache");
  server.send(200, "text/javascript; charset=utf-8", result);
}


void handleSysInfo() {
  String result;

  FSInfo fs_info;
  LittleFS.info(fs_info);

  result += "{\n";
  result += "  \"flashSize\": " + String(ESP.getFlashChipSize()) + ",\n";
  result += "  \"freeHeap\": " + String(ESP.getFreeHeap()) + ",\n";
  result += "  \"fsTotalBytes\": " + String(fs_info.totalBytes) + ",\n";
  result += "  \"fsUsedBytes\": " + String(fs_info.usedBytes) + ",\n";
  result += "}";

  server.sendHeader("Cache-Control", "no-cache");
  server.send(200, "text/javascript; charset=utf-8", result);
}


void buttonPush() {
  String queryName = (server.argName(0));
  String query = (server.arg(0));

  bool queryState;
  if(query == "0")
  {
    queryState = false;
  } else if (query == "1")
  {
    queryState = true;
  } else
  {
    TRACE("query value must be 0 or 1");
    queryState = false;
  }
  

  if(queryName=="B1")
  {
    stateButton1 = queryState;
  } else if (queryName=="B2")
  {
    stateButton2 = queryState;
  }

  getBackToButtonPageWithQuery();

  TRACE(("Button "+queryName+" is set to: "+query).c_str());
}


void setup(void) {
  delay(3000);
  Serial.begin(9600);

  TRACE("Mounting the filesystem...\n");
  if (!LittleFS.begin()) {
    TRACE("could not mount the filesystem...\n");
    delay(2000);
    ESP.restart();
  }

  // start WiFI
  WiFi.mode(WIFI_STA);
  if (strlen(ssid) == 0) {
    WiFi.begin();
  } else {
    WiFi.begin(ssid, passPhrase);
  }
  
  WiFi.setHostname(HOSTNAME);

  TRACE("Connect to WiFi...\n");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    TRACE(".");
  }
  TRACE("connected.\n");

  TRACE("Setup ntp...\n");
  configTime(TIMEZONE, "pool.ntp.org");


  // register a redirect handler when only domain name is given.
  server.on("/", HTTP_GET, handleRedirect);

  // register some REST services
  server.on("/$list", HTTP_GET, handleListFiles);
  server.on("/$sysinfo", HTTP_GET, handleSysInfo);
  server.on("/button.htm/push", buttonPush);

  // serve all static files
  server.serveStatic("/", LittleFS, "/");
    server.onNotFound([]() {
    server.send(404, "text/html", FPSTR(notFoundContent));
  });

   server.enableCORS(true);

  server.begin();
  String hostname = WiFi.getHostname();
  TRACE(("hostname = " + hostname).c_str());

  stateButton1 = false;
  stateButton2 = false;
}


void loop(void) {
  server.handleClient();
}

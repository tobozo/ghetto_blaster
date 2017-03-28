#define DEBUGOTA true

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
//#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <FS.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFSEditor.h>
//#include <TimeLib.h>

extern "C" {
  #include "user_interface.h"
}

//ESP8266WebServer server(80);

// SKETCH BEGIN
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
AsyncEventSource events("/events");


void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
  if(type == WS_EVT_CONNECT){
    Serial.printf("ws[%s][%u] connect\n", server->url(), client->id());
    client->printf("Hello Client %u :)", client->id());
    client->ping();
  } else if(type == WS_EVT_DISCONNECT){
    Serial.printf("ws[%s][%u] disconnect: %u\n", server->url(), client->id());
  } else if(type == WS_EVT_ERROR){
    Serial.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t*)arg), (char*)data);
  } else if(type == WS_EVT_PONG){
    Serial.printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len)?(char*)data:"");
  } else if(type == WS_EVT_DATA){
    AwsFrameInfo * info = (AwsFrameInfo*)arg;
    String msg = "";
    if(info->final && info->index == 0 && info->len == len){
      //the whole message is in a single frame and we got all of it's data
      Serial.printf("ws[%s][%u] %s-message[%llu]: ", server->url(), client->id(), (info->opcode == WS_TEXT)?"text":"binary", info->len);

      if(info->opcode == WS_TEXT){
        for(size_t i=0; i < info->len; i++) {
          msg += (char) data[i];
        }
      } else {
        char buff[3];
        for(size_t i=0; i < info->len; i++) {
          sprintf(buff, "%02x ", (uint8_t) data[i]);
          msg += buff ;
        }
      }
      Serial.printf("%s\n",msg.c_str());

      if(msg=="rotaryup") {
        RotaryTurnDetected = true;
        RotaryUp = true;
        
      } else if(msg=="rotarydown") {
        RotaryTurnDetected = true;
        RotaryUp = false;
        
      } else if(msg=="rotaryclick") {
        RotarySwitchDetected = true;
      }

      if(info->opcode == WS_TEXT)
        client->text("I got your text message");
      else
        client->binary("I got your binary message");
    } else {
      //message is comprised of multiple frames or the frame is split into multiple packets
      if(info->index == 0){
        if(info->num == 0)
          Serial.printf("ws[%s][%u] %s-message start\n", server->url(), client->id(), (info->message_opcode == WS_TEXT)?"text":"binary");
        Serial.printf("ws[%s][%u] frame[%u] start[%llu]\n", server->url(), client->id(), info->num, info->len);
      }

      Serial.printf("ws[%s][%u] frame[%u] %s[%llu - %llu]: ", server->url(), client->id(), info->num, (info->message_opcode == WS_TEXT)?"text":"binary", info->index, info->index + len);

      if(info->opcode == WS_TEXT){
        for(size_t i=0; i < info->len; i++) {
          msg += (char) data[i];
        }
      } else {
        char buff[3];
        for(size_t i=0; i < info->len; i++) {
          sprintf(buff, "%02x ", (uint8_t) data[i]);
          msg += buff ;
        }
      }
      Serial.printf("%s\n",msg.c_str());

      if((info->index + len) == info->len){
        Serial.printf("ws[%s][%u] frame[%u] end[%llu]\n", server->url(), client->id(), info->num, info->len);
        if(info->final){
          Serial.printf("ws[%s][%u] %s-message end\n", server->url(), client->id(), (info->message_opcode == WS_TEXT)?"text":"binary");
          if(info->message_opcode == WS_TEXT)
            client->text("I got your text message");
          else
            client->binary("I got your binary message");
        }
      }
    }
  }
}



/* Set these to your desired credentials. */
const char *ssid = "ghetto";
const char *password = "blaster1234";
const char* http_username = "admin";
const char* http_password = "admin";

// OTA updatable as long as the bool is true
bool otaready = true;
int otawait = 5000; // how long to wait for OTA at boot (millisec)
int otanow; // time counter for OTA update

void drawOta() {
    while( display.nextPage() );
}

void progressbar(uint8_t a) {
    display.drawFrame(0,50,120,10);
    display.drawVLine(a,51,8);
    drawOta();
}

void startWifi(){
  WiFi.disconnect(true);
  delay(300);
  wifi_set_phy_mode(PHY_MODE_11B);
  delay(300);
  WiFi.mode(WIFI_AP_STA);
  delay(300);
  WiFi.softAP(ssid, password);
  delay(300);

  Serial.println(" wifi done");
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP WebServer IP address: ");
  Serial.println(myIP);
  //server.on("/", handleRoot);
  //server.begin();
  Serial.println("HTTP server started");
}

void stopWifi() {
  WiFi.disconnect(true);
  delay(300);
  WiFi.mode(WIFI_OFF);
  delay(300);
}

void setupOta() {
  ESP.eraseConfig();
  display.firstPage();
  
  Serial.begin(115200);
  display.drawStr(10, 50, "OTA Check");
  progressbar(1);
  drawOta();

  #if DEBUGOTA==true
    Serial.println("OTA Check");
    //Serial.print("Setting Hostname: ");
    //Serial.println(ESPName);
    Serial.print("MAC Address:");
    Serial.printf("%02x", WiFi.macAddress()[0]);
    Serial.print(":");
    Serial.printf("%02x", WiFi.macAddress()[1]);
    Serial.print(":");
    Serial.printf("%02x", WiFi.macAddress()[2]);
    Serial.print(":");
    Serial.printf("%02x", WiFi.macAddress()[3]);
    Serial.print(":");
    Serial.printf("%02x", WiFi.macAddress()[4]);
    Serial.print(":");
    Serial.printf("%02x", WiFi.macAddress()[5]);
    Serial.println();
  #endif
  
  //WiFi.hostname("GhettoBlasterOTA");
  WiFi.mode(WIFI_STA);
  WiFi.begin("OAGUEST"); // see you on shodan.io :)
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    #if DEBUGOTA==true
      Serial.println("Connection Failed! Aborting...");
    #endif
    display.drawStr(10, 50, "OTA Fail, aborting");
    drawOta();
    delay(1000);
    otaready = false;
    stopWifi();
    startWifi();
    return;
  }

  progressbar(10);
  drawOta();

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    #if DEBUGOTA==true
      Serial.println("Start updating " + type);
    #endif
  });
  ArduinoOTA.onEnd([]() {
    #if DEBUGOTA==true
      Serial.println("\nEnd");
    #endif
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    #if DEBUGOTA==true
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    #endif
    //display.clear();
    display.drawStr(10, 50, "OTA Flashing");
    progressbar((progress / (total / 100)));
    //display.drawProgressBar(14, 27, 100, 10, 20);
    drawOta();
  });
  ArduinoOTA.onError([](ota_error_t error) {
    #if DEBUGOTA==true
      Serial.printf("Error[%u]: ", error);
    #endif
    //display.clear();
    display.drawStr(10, 50, "Error");
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
    otaready = false;
    drawOta();
  });

  ArduinoOTA.setHostname("GhettoBlaster");
  ArduinoOTA.begin();

  //display.clear();
  //display.drawProgressBar(14, 27, 100, 10, 20);
  progressbar(20);
  display.drawStr(10, 50, "OTA Ready");
  drawOta();
  #if DEBUGOTA==true
    Serial.println("Ready");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  #endif
  otanow = millis();
  drawOta();
}


void handleOta() {
  display.firstPage();
  
  int then = millis();
  if(then - otawait < otanow) {
    int percent = 100-(ceil(then - otanow) / (otawait / 100))+1;
    progressbar(percent);
    ArduinoOTA.handle();
  } else {
    otaready = false;
    stopWifi();
    startWifi();
    delay(300);

    MDNS.addService("http","tcp",80);
  
    SPIFFS.begin();
  
    ws.onEvent(onWsEvent);
    server.addHandler(&ws);
    events.onConnect([](AsyncEventSourceClient *client){
      client->send("hello!",NULL,millis(),1000);
    });
    server.addHandler(&events);
  
    server.addHandler(new SPIFFSEditor(http_username,http_password));
      
    server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(200, "text/plain", String(ESP.getFreeHeap()));
    });
  
    server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.htm");

    server.onNotFound([](AsyncWebServerRequest *request){
      Serial.printf("NOT_FOUND: ");
      if(request->method() == HTTP_GET)
        Serial.printf("GET");
      else if(request->method() == HTTP_POST)
        Serial.printf("POST");
      else if(request->method() == HTTP_DELETE)
        Serial.printf("DELETE");
      else if(request->method() == HTTP_PUT)
        Serial.printf("PUT");
      else if(request->method() == HTTP_PATCH)
        Serial.printf("PATCH");
      else if(request->method() == HTTP_HEAD)
        Serial.printf("HEAD");
      else if(request->method() == HTTP_OPTIONS)
        Serial.printf("OPTIONS");
      else
        Serial.printf("UNKNOWN");
      Serial.printf(" http://%s%s\n", request->host().c_str(), request->url().c_str());
  
      if(request->contentLength()){
        Serial.printf("_CONTENT_TYPE: %s\n", request->contentType().c_str());
        Serial.printf("_CONTENT_LENGTH: %u\n", request->contentLength());
      }
  
      int headers = request->headers();
      int i;
      for(i=0;i<headers;i++){
        AsyncWebHeader* h = request->getHeader(i);
        Serial.printf("_HEADER[%s]: %s\n", h->name().c_str(), h->value().c_str());
      }
  
      int params = request->params();
      for(i=0;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);
        if(p->isFile()){
          Serial.printf("_FILE[%s]: %s, size: %u\n", p->name().c_str(), p->value().c_str(), p->size());
        } else if(p->isPost()){
          Serial.printf("_POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
        } else {
          Serial.printf("_GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
        }
      }
  
      request->send(404);
    });
    
    server.onFileUpload([](AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final){
      if(!index)
        Serial.printf("UploadStart: %s\n", filename.c_str());
      Serial.printf("%s", (const char*)data);
      if(final)
        Serial.printf("UploadEnd: %s (%u)\n", filename.c_str(), index+len);
    });
    
    server.onRequestBody([](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
      if(!index)
        Serial.printf("BodyStart: %u\n", total);
      Serial.printf("%s", (const char*)data);
      if(index + len == total)
        Serial.printf("BodyEnd: %u\n", total);
    });
    server.begin();
  
    
  }
  
  drawOta();
}




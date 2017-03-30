/* 
 * Rotary controls for mini OLED Ghetto Blaster.
 * Based on examples from libraries for Catalex Serial MP3 Player and RDA5807M I2C FM Tuner.
 * 
 * Copyleft tobozo c(+) 2016/2017
 * 
 * Code was initally developed for Arduino Nano but quickly ran ouf or memory.
 * Compatibility is preserved for Arduino nano but new features will be 
 * developed for WemosD1 ESP8266 platform.
 *
 * PINOUT for Arduino Nano and Wemos MiniD1/ESP8266:
 *
 *  Arduino | WemosD1 | Catalex MP3
 *  -------------------------------
 *     10   |    D3   |    RX
 *     11   |    D4   |    TX
 *     5v   |    5v   |    VCC
 *     GND  |    GND  |    GND
 *          |         |    ROut ---> to NC1 @ DR21A01
 *          |         |    LOut ---> to NC2 @ DR21A01 
 *     GND  |    GND  |    GOut
 *
 *  Arduino | WemosD1 |  RDA5807M ***and*** SSD1306
 *  -----------------------------------------------
 *     A4   | SDA(D2) |    SDA
 *     A5   | SCL(D1) |    SCL
 *     3v   |   3v    |    VCC
 *     GND  |   GND   |    GND
 *
 *  Arduino | WemosD1 |  RDA5807M *only*
 *  ------------------------------------
 *          |         |    ROut ---> to NO1 @ DR21A01
 *          |         |    LOut ---> to NO2 @ DR21A01
 *          |         |    ANT  ---> to antenna
 *
 *  Arduino | WemosD1 |  Rotary Encoder
 *  -----------------------------------
 *     7    |    D6   |   CLK
 *     2    |    D5   |   DT
 *     3    |    D7   |   SW
 *     5v   |    5v   |   +
 *     GND  |    GND  |   GND
 *
 *  Arduino | WemosD1 |  DR21A01
 *  ----------------------------
 *     GND  |    GND  |   GND
 *     5v   |    5v   |   VCC
 *     12   |    D8   |   IN    
 *          |         |   COM1 ---> Speaker Out Right Channel
 *          |         |   COM2 ---> Speaker Out Left Channel
 *          |         |   NC1  ---> to ROut @ Catalex MP3
 *          |         |   NC2  ---> to LOut @ Catalex MP3
 *          |         |   NO1  ---> to ROut @ RDA5807M
 *          |         |   NO2  ---> to LOut @ RDA5807M
 * 
 */

#define debugblaster true

#ifdef ESP8266
#define ARDUINO_AUDIO_RELAY_PIN D8 // relay sound LOW/HIGH for DR21A01 DC 5V DPDT
#else
#define ARDUINO_AUDIO_RELAY_PIN 12 // relay sound LOW/HIGH for DR21A01 DC 5V DPDT
#endif

#include "rotarycontrols.h"
#include "radiocontrols.h" // FM Stereo Radio Module RDA5807M
#include "mp3controls.h"

#ifdef ESP8266
#include "wificontrols.h"
#endif

#define WIFI_MANAGER_USE_ASYNC_WEB_SERVER
#include <WiFiManager.h>


void setup() {
  Serial.begin(115200);
  
  initRotary();
  initDisplay();
  display.begin();
  display.setColorIndex(1); // init display  
  display.setFont(u8g_font_unifont);
  initMp3();
  initTuner();
  
  pinMode(ARDUINO_AUDIO_RELAY_PIN, OUTPUT); // init audio relay
  digitalWrite(ARDUINO_AUDIO_RELAY_PIN, HIGH);//DTDT relay activation
  //digitalWrite(ARDUINO_AUDIO_RELAY_PIN, LOW);//DTDT relay inactive

  #ifdef ESP8266
  setupOta();
  #endif

  
}

void loop() {
  
  #ifdef ESP8266
  if(otaready==true) {
    handleOta();
    return;
  }
  #endif
 
  handleMenu();

  #ifdef ESP8266
  /*
  if ((millis() - lasttick) > 1000) { //-- Demande toute les 1s  
    lasttick = millis();
    if(playing) {
      runMP3SerialCommand(CMD_PLAYING_N, 0x0000); //-- Demande numéro du fichier en cours de lecture
      delay(100);
      decodeMP3Answer();
      runMP3SerialCommand(CMD_GET_VOLUME, 0x0000);
      delay(100);
      decodeMP3Answer();
    }
    return;
  }*/

  if ((millis() - lastScrollTick) > scrollTickLength) {
    lastScrollTick = millis();
    ScrollTextPos = ScrollTextPos + scrollTickStep;
    repaint_needed = true;
  }
  #endif

  if(mp3Serial.available()) {
    decodeMP3Answer();
  }

  // TODO: if(draw_needed)
  if(repaint_needed) {
    display.firstPage();  
    do { draw(); } 
    while( display.nextPage() );
    repaint_needed = false;
  }


  #if debugblaster==true
  char c;
  if (Serial.available() > 0) {
    // read the next char from input.
    c = Serial.read();
    runTunerSerialCommand(c, 8930);
  } // if
  #endif
  
  #if USE_RDS == true
    radio.checkRDS();
  #endif 

}





// From http://www.mathertel.de/Arduino/RadioLibrary.aspx
// needs some tweaking to work with Arduino Nano without eating memory
#include <radio.h>

#define USE_RDS true


#include <RDA5807M.h>  // 0x3c
//#include <SI4703.h>
//#include <SI4705.h>
//#include <TEA5767.h>

RDA5807M radio; ///< Create an instance of a RDA5807 chip radio (i2c 0x3c)

#if USE_RDS == true
  #include <RDSParser.h>
  RDSParser rds; /// get a RDS parser
#endif

 /// State definition for this radio implementation.
enum SCAN_STATE {
  STATE_START,      ///< waiting for a new command character.
  STATE_NEWFREQ,
  STATE_WAITFIXED,
  STATE_WAITRDS,
  STATE_PRINT,
  STATE_END          ///< executing the command.
};

SCAN_STATE state; ///< The state variable is used for parsing input characters.

uint16_t g_block1;

// - - - - - - - - - - - - - - - - - - - - - - - - - -

#if USE_RDS == true
  // use a function inbetween the radio chip and the RDS parser
  // to catch the block1 value (used for sender identification)
  void RDS_process(uint16_t block1, uint16_t block2, uint16_t block3, uint16_t block4) {
    g_block1 = block1;
    rds.processData(block1, block2, block3, block4);
  }
#endif

void setTunerInfo() {
  RADIO_INFO info;
  radio.getRadioInfo(&info);
/*
  Serial.print("  RSSI: "); Serial.print(info.rssi);
  Serial.print("  SNR: "); Serial.print(info.snr);
  */
  char s[9];
  radio.formatFrequency(s, sizeof(s));
  SongMessage = String(s) + " " + String(info.rds ? "R" : "x") + String(info.tuned ? "T" : "x") + String(info.stereo ? "S" : "M");
  repaint_needed = true;
}


/// Update the ServiceName text on the LCD display.
void DisplayServiceName(char *name) {
  String out="";
  for (uint8_t n = 0; n < 8; n++) {
    if (name[n] != ' ') {
      //Serial.println("RDS:" + String(name));
      out += String(name);
    }
  }
  if(out!="") {
    ScrollText = out + " ";
    setTunerInfo();
  }

  return;
} // DisplayServiceName()


/// Execute a command identified by a character and an optional number.
/// See the "?" command for available commands.
/// \param cmd The command character.
/// \param value An optional parameter for the command.
void runTunerSerialCommand(char cmd, int16_t value) {

  unsigned long startSeek; // after 300 msec must be tuned. after 500 msec must have RDS.
  RADIO_FREQ fSave, fLast;
  RADIO_FREQ f = radio.getMinFrequency();
  RADIO_FREQ fMax = radio.getMaxFrequency();
  char sFreq[12];
      
  RADIO_INFO ri;
  if(cmd == 'r') {
    digitalWrite(ARDUINO_AUDIO_RELAY_PIN, LOW);//DTDT relay inactive
    //Serial.println(cmd);
  } else if (cmd == 'm') {
    digitalWrite(ARDUINO_AUDIO_RELAY_PIN, HIGH);//DTDT relay inactive
    //Serial.println(cmd);
  } else if (cmd == '+') { // ----- control the volume and audio output -----
    // increase volume
    uint8_t v = radio.getVolume();
    if (v < 15) radio.setVolume(++v);
  } else if (cmd == '-') {
    // decrease volume
    uint8_t v = radio.getVolume();
    if (v > 0) radio.setVolume(--v);
  } else if (cmd == 'u') {
    // toggle mute mode
    radio.setMute(!radio.getMute());
  } else if (cmd == 's') { 
     // toggle stereo mode
     radio.setMono(!radio.getMono()); 
  } else if (cmd == 'b') { 
    // toggle bass boost
    radio.setBassBoost(!radio.getBassBoost());
  } else if (cmd == '1') {
    // ----- control the frequency -----
    Serial.println("Scan(1)");
    fSave = radio.getFrequency();
    // start Simple Scan: all channels
    while (f <= fMax) {
      radio.setFrequency(f);
      delay(50);

      radio.getRadioInfo(&ri);

      #if debugblaster==true
        if (ri.tuned) {
          radio.formatFrequency(sFreq, sizeof(sFreq));
          Serial.print(sFreq);
          Serial.print(' ');
          Serial.print(ri.rssi); Serial.print(' ');
          Serial.print(ri.snr); Serial.print(' ');
          Serial.print(ri.stereo ? 'S' : '-');
          Serial.print(ri.rds ? 'R' : '-');
          Serial.println();
        } // if
      #endif
      // tune up by 1 step
      f += radio.getFrequencyStep();
    } // while
    radio.setFrequency(fSave);

  } else if (cmd == '2') {
    Serial.println("Seek(2)");
    fSave = radio.getFrequency();
    // start Scan
    radio.setFrequency(f);
    while (f <= fMax) {
      radio.seekUp(true);
      delay(100); // 
      startSeek = millis();
      // wait for seek complete
      do {
        radio.getRadioInfo(&ri);
      } while ((!ri.tuned) && (startSeek + 300 > millis()));
      // check frequency
      f = radio.getFrequency();
      if (f < fLast) {
        break;
      }
      fLast = f;

      #if debugblaster==true
        #if USE_RDS == true
          if ((ri.tuned) && (ri.rssi > 42) && (ri.snr > 12)) {
            radio.checkRDS();
            // print frequency.
            radio.formatFrequency(sFreq, sizeof(sFreq));
            Serial.print(sFreq);
            Serial.print(' ');
    
            do {
              radio.checkRDS();
              Serial.print(g_block1); Serial.print(' ');
            } while ((!g_block1) && (startSeek + 600 > millis()));
    
            // fetch final status for printing
            radio.getRadioInfo(&ri);
            Serial.print(ri.rssi); Serial.print(' ');
            Serial.print(ri.snr); Serial.print(' ');
            Serial.print(ri.stereo ? 'S' : '-');
            Serial.print(ri.rds ? 'R' : '-');
    
            if (g_block1) {
              Serial.print(' ');
              Serial.print('[');  Serial.print(g_block1, HEX); Serial.print(']');
            } // if
            Serial.println();
          } // if
        #endif
      #endif
      
    } // while
    radio.setFrequency(fSave);

  } else if (cmd == 'f') { 
    radio.setFrequency(value); 
  } else if (cmd == '.') { 
    radio.seekUp(false); 
  } else if (cmd == ':') { 
    radio.seekUp(true); 
    setTunerInfo();
  } else if (cmd == ',') { 
    radio.seekDown(false); 
  } else if (cmd == ';') { 
    radio.seekDown(true);
    setTunerInfo();
  } else if (cmd == '!') {
    // not in help:
    if (value == 0) radio.term();
    if (value == 1) radio.init();
  } else if (cmd == 'i') {
    char s[12];
    radio.formatFrequency(s, sizeof(s));
    SongMessage = s;
    Serial.print("Station:"); Serial.println(s);
    Serial.print("Radio:"); radio.debugRadioInfo();
    Serial.print("Audio:"); radio.debugAudioInfo();
    repaint_needed = true;
  } else if (cmd == 'x') {
    // info
    radio.debugStatus(); // print chip specific data.
  }

} // runTunerSerialCommand()



void initTuner() {
  radio.init();

  // Enable information to the Serial port
  radio.debugEnable(false);

  radio.setBandFrequency(RADIO_BAND_FM, 10550);

  // delay(100);

  radio.setMono(false);
  radio.setMute(false);
  // radio.debugRegisters();
  radio.setVolume(15);
  radio.setBassBoost(true);

  #if USE_RDS == true
    // setup the information chain for RDS data.
    radio.attachReceiveRDS(RDS_process);
    rds.attachServicenNameCallback(DisplayServiceName);
    Serial.println("Tuner RDS Init");
  #else
    Serial.println("Tuner Init");
  #endif

  setTunerInfo();
  
}



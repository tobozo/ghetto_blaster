#include <SoftwareSerial.h>

#ifdef ESP8266
#define ARDUINO_RX D4 // should connect to TX of the Serial MP3 Player module
#define ARDUINO_TX D3 // connect to RX of the module
SoftwareSerial mp3Serial(ARDUINO_RX, ARDUINO_TX);
#else
#define ARDUINO_RX 10 // should connect to TX of the Serial MP3 Player module
#define ARDUINO_TX 11 // connect to RX of the module
SoftwareSerial mp3Serial(ARDUINO_RX, ARDUINO_TX);
#endif

#define CMD_SNG_CYCL_PLAY 0X08  // Single Cycle Play.
#define CMD_SEL_DEV 0X09
#define DEV_TF 0X02
//#define CMD_PLAY_W_VOL 0X22
//#define CMD_PLAY_W_INDEX  0X03
#define CMD_PLAY 0X0D
#define CMD_PAUSE 0X0E
#define CMD_PLAYING_N     0x4C

#define CMD_STOP_PLAY     0X16
//#define CMD_FOLDER_CYCLE  0X17
#define CMD_SHUFFLE_PLAY  0x18 //
//#define CMD_SET_SNGL_CYCL 0X19 // Set single cycle.

#define CMD_SLEEP_MODE    0X0A
#define CMD_WAKE_UP       0X0B
#define CMD_RESET         0X0C
//#define CMD_PLAY_FOLDER_FILE 0X0F

#define CMD_PREVIOUS 0X02
#define CMD_NEXT 0X01

//#define CMD_VOLUME_UP     0X04
//#define CMD_VOLUME_DOWN   0X05
#define CMD_SET_VOLUME    0X06

//#define CMD_SET_DAC 0X1A
//#define DAC_ON  0X00
//#define DAC_OFF 0X01

#define CMD_GET_PLAYLIST_SIZE 0X48
#define CMD_GET_VOLUME 0X43

/************ Options **************************/ 
#define DEV_TF            0X02 
#define SINGLE_CYCLE_ON   0X00
#define SINGLE_CYCLE_OFF  0X01

#define SONG_ENDED_PREFIX "End#"
#define SONG_PLAYING_PREFIX "Play#"
#define VOLUME_PREFIX "Vol:"
#define SONG_PLAYING_SUFFIX " Songs"
#define PLAYER_STOPPED_MESSAGE "Stopped"
#define PLAYER_PAUSED_MESSAGE "Paused"

#define PLAYMODE_AUTOPLAY_ON "AutoON"
#define PLAYMODE_AUTOPLAY_OFF "AutoOFF"
#define LOOP_MODE_SINGLE_ON "Loop1ON"
#define LOOP_MODE_SINGLE_OFF "Loop1OFF"
//#define LABEL_WAKE "Wake"
//#define LABEL_SLEEP "Sleep"
#define LABEL_PAUSE "Pause"
#define LABEL_PLAY "Play"
#define LABEL_STOP "Stop"
#define LABEL_SHUFFLE "Random"
//#define LABEL_RESET "Reset"
#define LABEL_EXIT "Exit"
#define LABEL_MODE_MP3 "Mp3"
#define LABEL_MODE_TUNER "Tuner"

static uint8_t pre_vol, volume = 9; // Volume. 0-30 DEC values. 0x0f = 15. 
const static uint8_t volumeMin = 0, volumeMax = 30;
volatile boolean playing = false;    // Sending 'p' the module switch to Play to Pause or viceversa.

static int8_t Send_buf[8] = {0};
static int8_t Recv_buf[10] = {0};

//int16_t firstsong = 0X0F01;

volatile boolean dupe = false; // used as a switch to ignore duplicate signals
volatile boolean autoplay = true;
boolean autostart = true;
boolean isasleep = false;
boolean singlecycle = false;
boolean ignoredupe = true;

//boolean debug = true;

uint8_t playmode = 0; // 0 = full cycleplay, 1 = single cycle play, 
uint8_t playmodeMin = 0;
uint8_t playmodeMax = 6;
String playmodes[7] = {PLAYMODE_AUTOPLAY_ON, LOOP_MODE_SINGLE_OFF, /*LABEL_SLEEP, LABEL_RESET, */LABEL_PAUSE, LABEL_STOP, LABEL_SHUFFLE, LABEL_MODE_MP3, LABEL_EXIT};
/***************************
  0, // play with index
  1, // set single cycle play
  2, // close single cycle play
  3, // sleep
  4, // wake
  5, // reset
  6, // play
  7, // pause
  8, // stop
  9  // shuffle
****************************/

//boolean playmodesingle = false;

uint8_t totalsongs = 0;
//uint8_t totalfolders = 0;


volatile uint8_t menupos = 0; // 0 = tuner, 1 = song controls, 2 = mode, 3 = volume
const uint8_t menuposMin = 0, menuposMax = 2;
boolean inmenu = true;
boolean menuitemselected = false;

#ifdef ESP8266
#include "screencontrols_ESP.h"
#else
#include "screencontrols.h"
#endif

/********************************************************************************/
/*Function: Send command to the MP3                                         */
/*Parameter:-int8_t command                                                     */
/*Parameter:-int16_ dat  parameter for the command                              */
void runMP3SerialCommand(int8_t command, int16_t dat) {
  #if debug==true
    Serial.print("[SENDING] ");
    Serial.print(command, HEX);
    Serial.print(" / ");
    Serial.println(dat, HEX);
  #endif
  delay(20);
  Send_buf[0] = 0x7e;   // starting byte
  Send_buf[1] = 0xff;   // version
  Send_buf[2] = 0x06;   // Len
  Send_buf[3] = command;//
  Send_buf[4] = 0x01;   // 0x00 NO, 0x01 feedback
  Send_buf[5] = (int8_t)(dat >> 8);  //datah
  Send_buf[6] = (int8_t)(dat);       //datal
  Send_buf[7] = 0xef;   //
  for(uint8_t i=0; i<8; i++) {
    mp3Serial.write(Send_buf[i]) ;
  }
}


/********************************************************************************/
/*Function: sbyte2hex. Returns a byte data in HEX format.                 */
/*Parameter:- uint8_t b. Byte to convert to HEX.                                */
/*Return: String                                                                */
#if debug==true
String sbyte2hex(uint8_t b) {
  String shex;
  shex="0X";
  if (b < 16) shex+="0";
  shex+=String(b,HEX);
  shex+=" ";
  return shex;
}
#endif



void setVolume(uint8_t vol) {
  if(playmodes[5]==LABEL_MODE_MP3) {
    runMP3SerialCommand(CMD_SET_VOLUME, vol);
  } else {
    radio.setVolume((uint8_t)(vol/2));
    volume = radio.getVolume()*2;
  }
}

void playNextSong() {
  runMP3SerialCommand(CMD_NEXT, 0);
  runMP3SerialCommand(CMD_PLAYING_N, 0x0000); // ask for the number of file is playing
  delay(200);
}


void decodeMP3Answer(){

  uint8_t i = 0;

  // Get only 10 Bytes
  while(mp3Serial.available() && (i < 10)) {
    uint8_t b = mp3Serial.read();
    Recv_buf[i] = b;
    #if debug==true
    Serial.print(" 0x");
    Serial.print(b, HEX);
    #endif
    i++;
  }
  #if debug==true
  Serial.println();
  #endif
  
  switch (Recv_buf[3]) {
    case 0x3A:
      //decodedMP3Answer+=" -> Memory card inserted.";
    break;
    case 0x3D:
      //decodedMP3Answer+=" -> Completed play num "+String(Recv_buf[6],DEC);
      SongMessage = SONG_ENDED_PREFIX + String(Recv_buf[6],DEC);

      if(autoplay) {

        ignoredupe = !ignoredupe;
        if(ignoredupe) { // complete message seems to be fired twice, ignore odd ones
          // TODO: send async
          playNextSong();
        }
      } else {
        playing = false;
      }
    break;
    case 0x4C:
      //decodedMP3Answer+=" -> Playing: "+String(Recv_buf[6],DEC);
      SongMessage = SONG_PLAYING_PREFIX + String(Recv_buf[6],DEC);
      playing = true;
    break;
    //case 0x41:
      //decodedMP3Answer+=" -> OK";
      //decodedMP3Answer = "";
    //break;
    case 0x42:
      switch(Recv_buf[6]) {
        case 0x00:
          //decodedMP3Answer+=" -> Player stopped.";
          //SongMessage = PLAYER_STOPPED_MESSAGE;
          playing = false;
        break;
        case 0x01:
          //decodedMP3Answer+=" -> Player playing.";
          playing = true;
        break;
        case 0x02:
          //decodedMP3Answer+=" -> Player paused.";
          //SongMessage = PLAYER_PAUSED_MESSAGE;
          playing = false;
        break;
      }
    break;
    case 0x48:
      //decodedMP3Answer+="Found " + String(Recv_buf[6],DEC) + " Songs";
      SongMessage = String(Recv_buf[6],DEC) + SONG_PLAYING_SUFFIX;
      totalsongs = Recv_buf[6];
      // totalsongs
    break;
    case 0x43:
      //decodedMP3Answer+="Volume level is at " + String(Recv_buf[6],DEC);
      volume = Recv_buf[6];
      VolMessage = VOLUME_PREFIX + String(volume);
    break;
    /*
    case 0x4F:
      //decodedMP3Answer+="Found " + String(Recv_buf[6],DEC) + " Folders";
      //totalfolders = Recv_buf[6];
    break;
    */
  }
  repaint_needed = true;

} 



void handleMenu()  {

  if(RotarySwitchDetected) {
    if(millis() > Switchdebounce+500) {
      
      if(inmenu) {
        switch(menupos) {
          case 0: // volume controls
            inmenu = !inmenu;
          break;
          case 1: // song controls
            runMP3SerialCommand(CMD_PLAYING_N, 0x0000); // ask for the number of file is playing
            inmenu = !inmenu;
          break;
          case 2: // play mode controls
            inmenu = !inmenu;
          break;
        }
      } else {
        switch(menupos) {
          case 0: // volume controls
            inmenu = !inmenu;
          break;
          case 1: // song controls
            inmenu = !inmenu;
          break;
          case 2: // play mode controls
            if(menuitemselected) {
              menuitemselected = false;
              inmenu = !inmenu;
              switch(playmode) {
                case 0: 
                  autoplay = !autoplay; /* TODO: get index value from UI */ 
                  playmodes[playmode] = autoplay ? PLAYMODE_AUTOPLAY_ON : PLAYMODE_AUTOPLAY_OFF;
                break;
                case 1: 
                  if(!singlecycle) {
                    runMP3SerialCommand(CMD_SNG_CYCL_PLAY, SINGLE_CYCLE_ON);
                    singlecycle = true;
                    playmodes[playmode] = LOOP_MODE_SINGLE_ON;
                  } else {
                    runMP3SerialCommand(CMD_SNG_CYCL_PLAY, SINGLE_CYCLE_OFF);
                    singlecycle = false;
                    playmodes[playmode] = LOOP_MODE_SINGLE_OFF;
                  }
                break;
                /*
                case 2: 
                  if(!isasleep) {
                    runMP3SerialCommand(CMD_SLEEP_MODE, 0); 
                    isasleep = true;
                    playmodes[playmode] = LABEL_WAKE;
                  } else {
                    runMP3SerialCommand(CMD_WAKE_UP, 0); 
                    isasleep = false;
                    playmodes[playmode] = LABEL_SLEEP;
                  }
                break;
                case 3: runMP3SerialCommand(CMD_RESET, 0);break;
                */
                case 2: 
                 if(!playing) {
                   runMP3SerialCommand(CMD_PLAY, 0); 
                   playing = true; 
                   playmodes[playmode] = LABEL_PAUSE;
                 } else {
                   runMP3SerialCommand(CMD_PAUSE, 0); 
                   playing = false;
                   playmodes[playmode] = LABEL_PLAY;
                 }
                break;
                case 3: runMP3SerialCommand(CMD_STOP_PLAY, 0); playing = false; break;
                case 4: runMP3SerialCommand(CMD_SHUFFLE_PLAY, 0);break;
                case 5:
                  ScrollText = "";
                  if(playmodes[playmode]==LABEL_MODE_MP3) {
                    playmodes[playmode] = LABEL_MODE_TUNER;
                    digitalWrite(ARDUINO_AUDIO_RELAY_PIN, LOW);//DTDT relay inactive
                    runMP3SerialCommand(CMD_SLEEP_MODE, 0);
                    setTunerInfo();
                    playing = false;
                  } else {
                    playmodes[playmode] = LABEL_MODE_MP3;
                    digitalWrite(ARDUINO_AUDIO_RELAY_PIN, HIGH);//DTDT relay inactive
                    runMP3SerialCommand(CMD_WAKE_UP, 0);
                    delay(200);
                    runMP3SerialCommand(CMD_PLAY, 0);
                    ScrollText = "";
                    playing = true;
                  }
                  inmenu = !inmenu;
                  menupos = 1;
                break;
                case 6: ;break;
              }
              delay(200);
            } else {
              menuitemselected = true;
            }
            // TODO: visual feedback
          break;
        }
      }

      
      if(playing) {
        runMP3SerialCommand(CMD_PLAYING_N, 0x0000); //-- Demande numéro du fichier en cours de lecture
        delay(100);
        runMP3SerialCommand(CMD_GET_VOLUME, 0x0000);
        delay(100);
      }

      Switchdebounce = millis();
      repaint_needed = true;
      #if debug==true
        Serial.print (", autoplay = ");  
        Serial.print (autoplay);      
        Serial.print (", inmenu = ");  
        Serial.print (inmenu);
        Serial.print (", menupos = ");  
        Serial.println (menupos);
      #endif
    }
    RotarySwitchDetected = false;
  }
 
  if(RotaryTurnDetected)  {       // do this only if rotation was detected
    if(millis() > Rotarydebounce+500) {
      
      if (RotaryUp) {

        if(inmenu) {
          if(menupos+1>menuposMax) {
            menupos = menuposMin;
          } else {
            menupos++;
          }
        } else {
          switch(menupos) {
            case 2:
              if(playmode+1>playmodeMax) {
                playmode = playmodeMin;
              } else {
                playmode++;
              }
            break;
            case 1: // song controls
              if(playmodes[5]==LABEL_MODE_MP3) {
                playNextSong();
              } else {
                radio.seekUp(true);
                delay(200);
                ScrollText = "";
                setTunerInfo();
              }
            break;
            case 0: // TODO: tuner control
              if(volume+accel>volumeMax) {
                // ignore
              } else {
                //sendMP3Command('u');
                volume = volume + accel;
                volume++;
                setVolume(volume);
              }
              VolMessage = VOLUME_PREFIX + String(volume);
            break;
          }
        }
        
      } else {

        if(inmenu) {
          if(menupos-1<menuposMin) {
            menupos = menuposMax;
          } else {
            menupos--;
          }
        } else {
          switch(menupos) {
            case 2: // mode
              if(playmode-1<playmodeMin) {
                playmode = playmodeMax;
              } else {
                playmode--;
              }
            break;
            case 1: // song controls
              if(playmodes[5]==LABEL_MODE_MP3) {
                runMP3SerialCommand(CMD_PREVIOUS, 0);
                runMP3SerialCommand(CMD_PLAYING_N, 0x0000); // ask for the number of file is playing
                delay(200);
              } else {
                radio.seekDown(true);
                delay(200);
                ScrollText = "";
                setTunerInfo();
              }
            break;
            case 0: // volume control TODO: tuner control
              if(volume-accel<volumeMin) {
                // ignore
              } else {
                volume = volume - accel;
                //volume--;
                setVolume(volume);
              }
              VolMessage = VOLUME_PREFIX + String(volume);
            break;

          }
        }
        
      }
      
      Rotarydebounce = millis();
      repaint_needed = true;
      
      #if debug==true
        Serial.print (", inmenu = ");  
        Serial.print (inmenu);
        Serial.print (", menupos = ");  
        Serial.println (menupos);
      #endif

      accel = 1;
    } else {
      accel++;
      
    }
    RotaryTurnDetected = false;          // do NOT repeat IF loop until new rotation detected
    menuitemselected = false;
  }
}


void initMp3() {

  #ifdef ESP8266
  mp3Serial.begin(9600); // open com with mp3 player
  #else
  mp3Serial.begin(9600); // open com with mp3 player
  #endif

  delay(500); // Wait chip initialization is complete
  Serial.println("Mp3 ready");

  runMP3SerialCommand(CMD_SEL_DEV, DEV_TF);//select the TF card 
  delay(200); // wait for 200ms
  decodeMP3Answer();
  runMP3SerialCommand(CMD_GET_PLAYLIST_SIZE, 0x0000);
  delay(200); // wait for 200ms
  decodeMP3Answer();
  runMP3SerialCommand(CMD_PLAY, 0x0001);//play 
  delay(200); // wait for 200ms
  decodeMP3Answer();
  runMP3SerialCommand(CMD_GET_VOLUME, 0x0000);
  delay(200); // wait for 200ms
  decodeMP3Answer();
  runMP3SerialCommand(CMD_PLAYING_N, 0x0000); //-- Demande numéro du fichier en cours de lecture
  delay(200);
  decodeMP3Answer();

  VolMessage = VOLUME_PREFIX + String(volume);
  ModeMessage = playmodes[playmode];

}


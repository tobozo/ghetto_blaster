volatile boolean RotaryTurnDetected;
volatile boolean RotaryUp;
volatile boolean RotarySwitchDetected;
volatile boolean RotarySwitchPushed = false;

String VolMessage = "";
String SongMessage = "";
String ModeMessage = "";
String ScrollText = "";
boolean repaint_needed = true;

//unsigned long int lasttick = 0, lastScrollTick = 0, ScrollTextPos = 0;

//const uint8_t scrollTickStep = 1, scrollTickLength = 255;

#ifdef ESP8266
const uint8_t RotaryPinCLK=D6;                   // Used for generating interrupts using CLK signal
const uint8_t RotaryPinDT=D5;                    // Used for reading DT signal
const uint8_t RotaryPinSW=D7;                    // Used for the push button switch
#else
const uint8_t RotaryPinCLK=7;                   // Used for generating interrupts using CLK signal
const uint8_t RotaryPinDT=2;                    // Used for reading DT signal
const uint8_t RotaryPinSW=3;                    // Used for the push button switch
#endif

unsigned long Rotarydebounce = 0;
unsigned long Switchdebounce = 0;

void onRotaryClick()  {
  RotaryUp = (digitalRead(RotaryPinCLK) == digitalRead(RotaryPinDT));
  RotaryTurnDetected = true;
}

void onRotaryRotate()  {
  int RotarySwitchState = digitalRead(RotaryPinSW);
  if(RotarySwitchState!=RotarySwitchPushed) {
    RotarySwitchDetected = true;
    RotarySwitchPushed = RotarySwitchState;
  } else {
    RotarySwitchDetected = false;
  }
}

void initRotary() {
  pinMode(RotaryPinCLK,INPUT);
  pinMode(RotaryPinDT,INPUT);  
  pinMode(RotaryPinSW,INPUT_PULLUP);
  attachInterrupt (digitalPinToInterrupt(RotaryPinDT), onRotaryClick, CHANGE);
  attachInterrupt (digitalPinToInterrupt(RotaryPinSW), onRotaryRotate, CHANGE);
  #if debug==true
    Serial.println("Rotary listening");
  #endif
}

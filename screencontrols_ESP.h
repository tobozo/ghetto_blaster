
#include "U8g2lib.h"
U8G2_SSD1306_128X64_NONAME_F_SW_I2C display(U8G2_R0, /* clock=*/ SCL, /* data=*/ SDA, /* reset=*/ U8X8_PIN_NONE);   // All Boards without Reset of the Display

#define VOLUME_LABEL "Vol"
#define MODE_LABEL "Mode"
#define SONG_LABEL "Song"
#define FREQ_LABEL "Freq"

unsigned long lasttick, lastScrollTick, scrollTickLength = 50;
int ScrollTextPos = 0, scrollTickStep = 1;
int scrollTextCharHeight = 13, scrollTextCharWidth = 8;

int labelYpos = 35; // y position of the middle menu text


void drawArrows() {
  display.drawTriangle(4,   labelYpos-6, 10,  labelYpos-12, 10,  labelYpos);
  display.drawTriangle(124, labelYpos-6, 118, labelYpos-12, 118, labelYpos);
}



void drawModeConsole() {
  
  ModeMessage = playmodes[playmode];
  
  /*
  switch(playmode) {
    case 0:      ModeMessage = "loop all";    break;
    case 1:      ModeMessage = "loop one on";    break;
    case 2:      ModeMessage = "loop one off";    break;
    case 3:      ModeMessage = "sleep";    break;
    case 4:      ModeMessage = "wake";    break;
    case 5:      ModeMessage = "reset";    break;
    case 6:      ModeMessage = "play";    break;
    case 7:      ModeMessage = "pause";    break;
    case 8:      ModeMessage = "stop";    break;
    case 9:      ModeMessage = "shuffle";    break;
  }*/
  //display.setFont(u8g_font_unifont);
  int labelXpos = (128 - (ModeMessage.length() * scrollTextCharWidth))/2;
  
  if(menuitemselected) {
    display.drawBox(16,labelYpos-12, 96,15);
    display.setColorIndex(0);
    display.drawStr(labelXpos, labelYpos, ModeMessage.c_str());
    display.setColorIndex(1);
  } else {
    display.drawStr(labelXpos, labelYpos, ModeMessage.c_str());
  }
}
/*
void drawTunerConsole() {
  display.setFont(u8g_font_unifont);
  display.drawStr( 2, 30, "Tuner");
}*/


void drawVolumeConsole() {
  display.drawStr( 2, labelYpos, VolMessage.c_str());
}

void drawScrollText() {
  if(ScrollText=="") return;
  int textwidth = (ScrollText.length()+1) * scrollTextCharWidth;
  int offset = (128 - textwidth) / 2;
  int leftmaskpos = offset;
  int rightmaskpos = offset + textwidth - scrollTextCharWidth;
  display.setFont(u8g_font_unifont);
  display.drawStr(offset + ScrollTextPos, 54, ScrollText.c_str());
  display.setColorIndex(0);
  display.drawBox(leftmaskpos,  56-scrollTextCharHeight, scrollTextCharWidth, scrollTextCharHeight);
  display.drawBox(rightmaskpos, 56-scrollTextCharHeight, scrollTextCharWidth, scrollTextCharHeight);
  display.setColorIndex(1);
}

void drawSongConsole() {
  //display.setFont(u8g_font_unifont);
  if(playmodes[playmode]==LABEL_MODE_TUNER) {
    setTunerInfo();
  }
  int labelXpos = (128 - (SongMessage.length() * scrollTextCharWidth))/2;
  display.drawStr( labelXpos, labelYpos, SongMessage.c_str());
}



void scrollText() {
  ScrollTextPos--;
  if(ScrollTextPos<=0) {
    ScrollTextPos = scrollTextCharWidth;
    int lastcharpos = ScrollText.length();
    if(lastcharpos<=0) return;
    //ScrollText = ScrollText.substring(lastcharpos-1, lastcharpos) + ScrollText.substring(0, lastcharpos-1);
    ScrollText = ScrollText.substring(1, lastcharpos) + ScrollText.substring(0, 1);
  }
}


void drawMenu() {
  //display.setFont(u8g_font_unifont);
  
  if(menupos==0) {
    display.drawBox(0,0,28,16);
    display.setColorIndex(0);
    display.drawStr( 2, 13, VOLUME_LABEL);
    display.setColorIndex(1);
  } else {
    display.drawStr( 2, 13, VOLUME_LABEL);
    display.drawLine(28, 0, 28, 16);
  }

  if(menupos==1) {
    display.drawBox(28,0, 36,16);
    display.setColorIndex(0);
    if(playmodes[5]==LABEL_MODE_MP3) {
      display.drawStr( 30, 13, SONG_LABEL);
    } else {
      display.drawStr( 30, 13, FREQ_LABEL);
    }    
    display.setColorIndex(1);
  } else {
    if(playmodes[5]==LABEL_MODE_MP3) {
      display.drawStr( 30, 13, SONG_LABEL);
    } else {
      display.drawStr( 30, 13, FREQ_LABEL);
    }
    display.drawLine(64, 0, 64, 16);
  }

  if(menupos==2) {
    display.drawBox(64,0, 36,16);
    display.setColorIndex(0);
    display.drawStr( 66, 13, MODE_LABEL);
    display.setColorIndex(1);
    //display.drawStr( ScrollTextPos, 54, ScrollText.c_str());
    
  } else {
    display.drawStr( 66, 13, MODE_LABEL);
    display.drawLine(100, 0, 100, 16);
  }
/*
  if(menupos==3) {
    display.drawBox(0,0,28,16);
    display.setColorIndex(0);
    display.drawStr( 2, 13, VOLUME_LABEL);
    display.setColorIndex(1);
  } else {
    display.drawStr( 2, 13, VOLUME_LABEL);
    display.drawLine(28, 0, 28, 16);
  }
*/
  if(inmenu) {
    display.drawFrame(0, 0, 128, 17);
  } else {
    display.drawFrame(0, labelYpos-14, 128, 19);
  }
}

void drawIcons() {
  int x = 102;
  int y = 4;
  
  display.drawBox(x,y+3, 5,3);
  display.drawTriangle(x+2,y+3, x+5,y,   x+5, y+3);
  display.drawTriangle(x+2,y+5, x+5,y+5, x+5, y+8);
  
  if(volume==0) {
    // display a cross
    display.drawLine(x+6, y+2, x+10, y+6);
    display.drawLine(x+10, y+2, x+6, y+6);
  } else {
    for(uint8_t i=volumeMin+10;i<volume+10;i=i+10) {
      display.drawCircle(x+6, y+4, i/6, U8G2_DRAW_UPPER_RIGHT);
      display.drawCircle(x+6, y+4, i/6, U8G2_DRAW_LOWER_RIGHT);
    }
  }

  if(playing) {
    display.drawTriangle( 116, 4, 116, 12, 124, 8 );
    if(autoplay) {
      display.setColorIndex(0);
      display.drawTriangle( 118, 6, 118, 10, 122, 8 );
      display.setColorIndex(1);
    }
  } else {
    // TODO: difference between paused, stopped (square), sleep (z)
    display.drawBox( 116, 4, 2, 8 );
    display.drawBox( 120, 4, 2, 8 );
  }
  
}


void drawVolumeBar() {
  uint8_t barHeight = 3;
  uint8_t iconWidth = 14;
  uint8_t x = 77, y=25;

  display.drawFrame(x, y, 48, 11);

  x = x+2;
  y = y+1;

  // draw icon
  /*   012345
     0 .....#
     1 ....##
     2 ...###
     3 ######   
     4 ######
     5 ######
     6 ...###
     7 ....##
     8 .....#      */
   display.drawBox(x,y+3, 5,3);
   display.drawTriangle(x+2,y+3, x+5,y,   x+5, y+3);
   display.drawTriangle(x+2,y+5, x+5,y+5, x+5, y+8);

  // draw volume box
  display.drawBox(x+iconWidth, y+barHeight, volume, barHeight);

  if(volume==0) {
    // display a cross
    display.drawLine(x+6, y+2, x+10, y+6);
    display.drawLine(x+10, y+2, x+6, y+6);
  } else {
    for(uint8_t i=volumeMin+10;i<volume+10;i=i+10) {
      display.drawCircle(x+6, y+4, i/6, U8G2_DRAW_UPPER_RIGHT);
      display.drawCircle(x+6, y+4, i/6, U8G2_DRAW_LOWER_RIGHT);
    }
  }
}

void draw() {
  drawMenu();
  drawIcons();
  
  switch(menupos) {
    case 2:
      drawArrows();
      drawModeConsole();
    break;
    case 1:
      drawArrows();
      drawSongConsole();
    break;
    case 0:
      drawVolumeConsole();
      drawVolumeBar();
    break;
  }
  
  drawScrollText();

}


void initDisplay() {
  display.begin();
  display.setColorIndex(1); // init display  
  display.setFont(u8g_font_unifont);
  display.setDisplayRotation(U8G2_R2);
}




Ipod like controls for Arduino Nano or Wemos MiniD1.

Components:

  - Wemos Mini D1
  - Tuner RDA5807M I2C 
  - Catalex Serial MP3 Player
  - DR21A01 DC 5V DPDT Relay Module (2 chan audio)
  - Rotary Encoder (+switch)
  - 0.96'' OLED SSD1306 (I2C)


Most of this code is Based on examples from libraries for Catalex Serial MP3 Player and RDA5807M I2C FM Tuner.
  
The code was initally developed for Arduino Nano but it quickly ran ouf or memory.
Compatibility will be preserved for Arduino nano but new features will be developed for WemosD1 ESP8266 platform only.
 
PINOUT for Arduino Nano and Wemos MiniD1/ESP8266:

 ```
   Arduino | WemosD1 | Catalex MP3
   -------------------------------
      10   |    D3   |    RX
      11   |    D4   |    TX
      5v   |    5v   |    VCC
      GND  |    GND  |    GND
           |         |    ROut ---> to NC1 @ DR21A01
           |         |    LOut ---> to NC2 @ DR21A01 
      GND  |    GND  |    GOut
 
   Arduino | WemosD1 |  RDA5807M ***and*** SSD1306
   -----------------------------------------------
      A4   | SDA(D2) |    SDA
      A5   | SCL(D1) |    SCL
      3v   |   3v    |    VCC
      GND  |   GND   |    GND
 
   Arduino | WemosD1 |  RDA5807M *only*
   ------------------------------------
           |         |    ROut ---> to NO1 @ DR21A01
           |         |    LOut ---> to NO2 @ DR21A01
           |         |    ANT  ---> to antenna
 
   Arduino | WemosD1 |  Rotary Encoder
   -----------------------------------
      7    |    D6   |   CLK
      2    |    D5   |   DT
      3    |    D7   |   SW
      5v   |    5v   |   +
      GND  |    GND  |   GND
 
   Arduino | WemosD1 |  DR21A01
   ----------------------------
      GND  |    GND  |   GND
      5v   |    5v   |   VCC
      12   |    D8   |   IN    
           |         |   COM1 ---> Speaker Out Right Channel
           |         |   COM2 ---> Speaker Out Left Channel
           |         |   NC1  ---> to ROut @ Catalex MP3
           |         |   NC2  ---> to LOut @ Catalex MP3
           |         |   NO1  ---> to ROut @ RDA5807M
           |         |   NO2  ---> to LOut @ RDA5807M
  
  ```


Prototype #1 (only mp3 player)

  [![Prototype 1](https://img.youtube.com/vi/2QCEO0HXd_0/0.jpg)](https://www.youtube.com/watch?v=2QCEO0HXd_0)


Prototype #2 (added tuner, no wiring yet, sudo make me a sandwich)

  [![Prototype 2](https://img.youtube.com/vi/UQzi_qFmI6I/0.jpg)](https://www.youtube.com/watch?v=UQzi_qFmI6I)
  
  
  
  

//
//
//   Ian Lasky
//   March 23, 2017
//
//
//   Outline of clock for bitmapping reference
//
//          0 1 2 3 4 5 6 7 8 9 10
//        0 I T S X A S H A L F H
//        1 T W E N T Y F I V E A
//        2 Q U A R T E R T E N P
//        3 T I L P A S T O N E P
//        4 T W O B I R T H D A Y
//        5 G T H R E E S E V E N
//        6 F O U R F I V E S I X
//        7 Y O U R N A M E T E N
//        8 N I N E I G H T M I D
//        9 Z E L E V E N O O N J
//       10 N I G H T O C L O C K
//
#include <Time.h>
#include <TimeLib.h>
#include <Wire.h>
#include <DS3232RTC.h>
#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>

#ifdef __AVR__
  #include <avr/power.h>
#endif

#define ROWS 11
#define COLUMNS 11

//---------------------------------------------------------------
//Bit map for each led, 11-rows
uint16_t wordMap[ROWS];   

//Different clock modes
enum modes
{
  OFF, 
  WORD_MODE, 
  DIGIT_MODE, 
  CYCLE_MODE,
  ADJUST_COLOR,
  ADJUST_BRIGHTNESS,
  ADJUST_HOUR,
  ADJUST_MINUTE,
  ADJUST_MONTH,
  ADJUST_DAY,
  ADJUST_YEAR
};

//Create mode variable
modes mode;

//Mode index for mode switching
int modeDex;

//Variables for adjustable word clock color
uint16_t wordColor, savedColor; 
int colorPos;

//LED locations on color wheel to select color
uint16_t colorWheel[28][2] = {{0,3},{0,4},{0,5},{0,6},{0,7},{1,8},{2,9},
                              {3,10},{4,10},{5,10},{6,10},{7,10},{8,9},{9,8},
                              {10,7},{10,6},{10,5},{10,4},{10,3},{9,2},{8,1},
                              {7,0},{6,0},{5,0},{4,0},{3,0},{2,1},{1,2}};

//Variables for adjustable word clock brightness
uint8_t brightness;
int radius;

//LED Locations for HAPPY BIRTHDAY YOURNAME
uint16_t birthdayLEDS[20][2] = {{10,0},{10,1},{10,2},{10,3},{3,4},              
                                {4,4},{5,4},{6,4},{7,4},{8,4},{9,4},{10,4},     
                                {0,7},{1,7},{2,7},{3,7},{4,7},{5,7},{6,7},{7,7}}; 

int upPushedPrev;
int downPushedPrev;
int selectPushedPrev;



//---------------------------------------------------------------

//Addressable LED strip pin
#define PIN 6

//Up, down, and select buttons configured as pullup buttons
#define SELECT 2
#define DOWN 3
#define UP 4


//Bit mapping the words to corresponding LEDs
//Used hexadecimal, but the binary on the right shows
//why the hexidecimal value is its value
//e.g., 0x700 = 0b11100000000 corresponds to the first 
//3 LEDs of the first (zeroth) line of the matrix. 
#define keyITS       wordMap[0]  |= 0x700                //0b11100000000
#define keyA         wordMap[0]  |= 0x40                 //0b00001000000

#define keyFIVE      wordMap[1]  |= 0x1e                 //0b00000011110
#define keyTEN       wordMap[2]  |= 0xe                  //0b00000001110
#define keyTWENTY    wordMap[1]  |= 0x7e0                //0b11111100000
#define keyQUARTER   wordMap[2]  |= 0x7f0                //0b11111110000
#define keyHALF      wordMap[0]  |= 0x1e                 //0b00000011110

#define keyTIL       wordMap[3]  |= 0x700                //0b11100000000
#define keyPAST      wordMap[3]  |= 0xf0                 //0b00011110000
#define keyOCLOCK    wordMap[10] |= 0x3f                 //0b00000111111

//#define keyHAPPY     wordMap[0]  |= 0x1, wordMap[1]  |= 0x1, wordMap[2]  |= 0x1, wordMap[3]  |= 0x1, wordMap[4]  |= 0x1
//#define keyBIRTHDAY  wordMap[4]  |= 0xff                //0b00011111111
//#define keyYOURNAME  wordMap[7]  |= 0x7f8               //0b11111111000


#define hourONE      wordMap[3]  |= 0xe                 //0b00000001110
#define hourTWO      wordMap[4]  |= 0x700               //0b11100000000
#define hourTHREE    wordMap[5]  |= 0x3e0               //0b01111100000
#define hourFOUR     wordMap[6]  |= 0x780               //0b11110000000
#define hourFIVE     wordMap[6]  |= 0x78                //0b00001111000
#define hourSIX      wordMap[6]  |= 0x7                 //0b00000000111
#define hourSEVEN    wordMap[5]  |= 0x1f                //0b00000011111
#define hourEIGHT    wordMap[8]  |= 0xf8                //0b00011111000
#define hourNINE     wordMap[8]  |= 0x780               //0b11110000000
#define hourTEN      wordMap[7]  |= 0x7                 //0b00000000111
#define hourELEVEN   wordMap[9]  |= 0x3f0               //0b01111110000
#define hourNOON     wordMap[9]  |= 0x1e                //0b00000011110
#define hourMIDNIGHT wordMap[8]  |= 0x7, wordMap[10]  |= 0x7c0       //0b00000000111 , 0b11111000000

//11 modes as listed in the enum above
#define MODENUM 11


// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
//Adafruit_NeoPixel strip = Adafruit_NeoPixel(150, PIN, NEO_GRB + NEO_KHZ800);

Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(ROWS, COLUMNS, PIN,
      NEO_MATRIX_BOTTOM  + NEO_MATRIX_LEFT +
      NEO_MATRIX_COLUMNS + NEO_MATRIX_ZIGZAG,
      NEO_GRB         + NEO_KHZ800);

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

void setup() 
{


  Serial.begin(9600);

  //Initialize pins as pullup buttons. 
  //Works by connection button positive to pin number
  //and button negative to ground. HIGH when inactive,
  //LOW when active
  pinMode(UP,INPUT_PULLUP);
  pinMode(DOWN,INPUT_PULLUP);
  pinMode(SELECT,INPUT_PULLUP);

  //Start the matrix and clear it
  matrix.begin();
  matrix.show();

  //Initialize radius for brightness star
  //Brightness is from 0-5
  radius = 3;
  brightness = radius * 256 / 5;
  matrix.setBrightness(brightness);

  //Initalize the start mode as WORDMODE
  modeDex = 1;
  mode = modes{modeDex};

  upPushedPrev = LOW;
  downPushedPrev = LOW;
  selectPushedPrev = LOW;




  //Uncomment line to set time. Then comment and reupload.
  //This prevents it from changing the time again when power
  //is disconnected. 
  //setTime(hour,minute,second,day,month,year)
//  setTime(8,53,00,18,2,2017);
//  RTC.set(now());

  //Initialize the color position on the wheel to the 
  //0 position
  //28 possible colors 
  colorPos = 0;
  savedColor = Wheel(((colorPos+1) * 256/ 28) % 256);
  wordColor = savedColor;

  
}

void loop() 
{

    //Create tmElements_t variable for storing RTC info
    tmElements_t tm;
    RTC.read(tm);

    //Set variables for up, down, and select buttons
    //Digital read means they will read from digital pins
    //2, 3, and 4 respectively
    int upPushed = digitalRead(4);
    int downPushed = digitalRead(3);
    int selectPushed = digitalRead(2);

    //Use the select key to cycle between modes
    //Only able to go one direction, but easier than
    //Coding the up and down to select modes
    if(!selectPushed)
    //if(selectPushed == 32)
    {
      if(selectPushed != selectPushedPrev)
      {
        modeDex++;
        modeDex%=MODENUM;
        mode = modes{modeDex};
        selectPushedPrev = selectPushed;
      }

    }
    else
    {
      if(selectPushed != selectPushedPrev)
      {
        selectPushedPrev = selectPushed;
      }
    }

    if(!upPushed && ! downPushed)
    {
      secret();
    }
    


   
    
     
    Serial.print(tm.Hour,DEC);
    Serial.print(":");
    Serial.print(tm.Minute,DEC);
    Serial.print(":");
    Serial.print(tm.Second,DEC);
    Serial.print(" ");
    Serial.print(tm.Month,DEC);
    Serial.print("/");
    Serial.print(tm.Day,DEC);
    Serial.print("/");
    Serial.println(1970+tm.Year,DEC);

    //Set the word time using the current time information
    wordTime(tm);

  
    //Switch statement to determine the current mode
    switch(mode)
    {
      case OFF:
        Serial.println("OFF");
        //Setting the word color to 0 turns off the LEDs
        wordColor = 0;     
        showWordMap();
        break;
      case WORD_MODE:
        Serial.println("WORD_MODE");
        wordColor = savedColor;
        showWordMap();
        if(tm.Day == 4 && tm.Month == 7)
        {
          birthday();
        }
        break;
      case DIGIT_MODE:
        Serial.println("DIGIT_MODE");
        showDigitMap(tm);
        matrix.setBrightness(brightness);
        break;
      case CYCLE_MODE:
        Serial.println("CYCLE_MODE");
        // Cycle between word mode and digit mode every 10 seconds
        // (condition) ? true case : false case
        tm.Second % 20 > 10 ? showWordMap() : showDigitMap(tm);
        if((tm.Second % 20 > 10) && (tm.Day == 4) && (tm.Month == 7))
        {
          birthday();
        }
        matrix.setBrightness(brightness);
        break;
      case ADJUST_COLOR:
        Serial.println("ADJUST_COLOR");
        adjustColor(upPushed,downPushed);
        upPushedPrev = upPushed;
        //checkPushed(upPushed,downPushed);    
        //adjustColor(selectPushed);
        break;
      case ADJUST_BRIGHTNESS:
        Serial.println("ADJUST_BRIGHTNESS");
        adjustBrightness(upPushed,downPushed);
        //checkPushed(upPushed,downPushed); 
        //adjustBrightness(selectPushed);
        break;
      case ADJUST_HOUR:
        Serial.println("ADJUST_HOUR");
        adjustHour(upPushed == HIGH,downPushed == HIGH, tm);
        //adjustHour(selectPushed, tm);
        break;
      case ADJUST_MINUTE:
        Serial.println("ADJUST_MINUTE");
        adjustMinute(upPushed == HIGH,downPushed == HIGH, tm);
        //adjustMinute(selectPushed,tm);
        break;
      case ADJUST_MONTH:
        Serial.println("ADJUST_MONTH");
        adjustMonth(upPushed == HIGH,downPushed == HIGH, tm);
        //adjustMonth(selectPushed,tm);
        break;
      case ADJUST_DAY:
        Serial.println("ADJUST_DAY");
        adjustDay(upPushed == HIGH,downPushed == HIGH, tm);
        //adjustDay(selectPushed,tm);
        break;
      case ADJUST_YEAR:
        Serial.println("ADJUST_YEAR");
        adjustYear(upPushed == HIGH,downPushed == HIGH, tm);
        break;
        
    }



    /*Serial.write(27);       // ESC command
    Serial.print("[2J");    // clear screen command
    Serial.write(27);
    Serial.print("[H");     // cursor to home command

   
    //delay(1);*/

}

void secret()
{
  int x = matrix.width();
  matrix.setTextColor(savedColor);
  matrix.setTextWrap(false);
  while (--x > -100)
  {
    matrix.fillScreen(0);
    matrix.setCursor(x,2);
    matrix.print(F("Hey, beautiful"));
    matrix.show();
    delay(100);
  }
}

void checkPushed(int upPushed, int downPushed)
{
  upPushedPrev = (upPushedPrev != upPushed) ? upPushed : upPushedPrev;
  downPushedPrev = (downPushedPrev != downPushed) ? downPushed : downPushedPrev;
}

void adjustYear(int upPushed, int downPushed, tmElements_t tm)
{
  int units, tens;
  units = (tm.Year + 1970)  % 10;
  tens  = ((tm.Year + 1970)  / 10) % 10;
  if(!upPushed)
  //if(selectPushed == 48)
  {
    if(upPushed != upPushedPrev)
    {
      tm.Year = (tm.Year + 1 > 130) ? 30 : tm.Year + 1;
      RTC.write(tm);
    }
  }
  else if(!downPushed)
  //else if(selectPushed == 49)
  {
    if(downPushed != downPushedPrev)
    {
      tm.Year = (tm.Year - 1 < 30) ? 129 : tm.Year - 1;
      RTC.write(tm);
    }
  }
  upPushedPrev = upPushed;
  downPushedPrev = downPushed;
  matrix.clear();
  
  matrix.drawChar( 0, 2, tens+48, savedColor, 0x0000,1);
  matrix.drawChar( 6, 2, units+48, savedColor, 0x0000,1);
  matrix.show();
}

void adjustDay(int upPushed, int downPushed, tmElements_t tm)
//void adjustDay(int selectPushed, tmElements_t tm)
{
  
  int units, tens;
  units = tm.Day  % 10;
  tens  = tm.Day  / 10;
  if(!upPushed)
  //if(selectPushed == 48)
  {
    if(upPushed != upPushedPrev)
    {
      tm.Day = (tm.Day + 1 > 31) ? 1 : tm.Day + 1;
      RTC.write(tm);
    }
  }
  else if(!downPushed)
  //else if(selectPushed == 49)
  {
    if(downPushed != downPushedPrev)
    {
      tm.Day = (tm.Day - 1 < 1) ? 31 : tm.Day - 1;
      RTC.write(tm);
    }
  }
  upPushedPrev = upPushed;
  downPushedPrev = downPushed;
  matrix.clear();

  Serial.println(tens);
  
  matrix.drawChar( 0, 2, tens+48, savedColor, 0x0000,1);
  matrix.drawChar( 6, 2, units+48, savedColor, 0x0000,1);
  matrix.show();
}

void adjustMonth(int upPushed, int downPushed, tmElements_t tm)
//void adjustMonth(int selectPushed, tmElements_t tm)
{
  int units, tens;
  units = tm.Month  % 10;
  tens  = tm.Month  / 10;
  if(!upPushed)
  //if(selectPushed == 48)
  {
    if(upPushed != upPushedPrev)
    {
      tm.Month = (tm.Month + 1 > 12) ? 1 : tm.Month + 1;
      RTC.write(tm);
    }
  }
  else if(!downPushed)
  //else if(selectPushed == 49)
  {
    if(downPushed != downPushedPrev)
    {
      tm.Month = (tm.Month - 1 < 1) ? 12 : tm.Month - 1;
      RTC.write(tm);
    }
  }
  upPushedPrev = upPushed;
  downPushedPrev = downPushed;
  matrix.clear();
  
  matrix.drawChar( 0, 2, tens+48, savedColor, 0x0000,1);
  matrix.drawChar( 6, 2, units+48, savedColor, 0x0000,1);
  matrix.show();
}

void adjustHour(int upPushed, int downPushed, tmElements_t tm)
//void adjustHour(int selectPushed, tmElements_t tm)
{
  int units, tens;
  units = tm.Hour  % 10;
  tens  = tm.Hour  / 10;
  if(!upPushed)
  //if(selectPushed == 48)
  {
    if(upPushed != upPushedPrev)
    {
      tm.Hour = (tm.Hour + 1 > 23) ? 0 : tm.Hour + 1;
      RTC.write(tm);
    }
  }
  else if(!downPushed)
  //else if(selectPushed == 49)
  {
    if(downPushed != downPushedPrev)
    {
      tm.Hour = (tm.Hour - 1 < 0) ? 23 : tm.Hour - 1;
      RTC.write(tm);
    }
  }
  upPushedPrev = upPushed;
  downPushedPrev = downPushed;
  matrix.clear();
 
  matrix.drawChar( 0, 2, tens+48, savedColor, 0x0000,1);
  matrix.drawChar( 6, 2, units+48, savedColor, 0x0000,1);
  matrix.show();
}

void adjustMinute(int upPushed, int downPushed, tmElements_t tm)
//void adjustMinute(int selectPushed, tmElements_t tm)
{
  int units, tens;
  units = tm.Minute  % 10;
  tens  = tm.Minute  / 10;
  if(!upPushed)
  //if(selectPushed == 48)
  {
    if(upPushed != upPushedPrev)
    {
      tm.Minute = (tm.Minute + 1 > 59) ? 0 : tm.Minute + 1;
      tm.Second = 0;
      RTC.write(tm);
    }
  }
  else if(!downPushed)
  //else if(selectPushed == 49)
  {
    if(downPushed != downPushedPrev)
    {
      tm.Minute = (tm.Minute - 1 < 0) ? 59 : tm.Minute - 1;
      tm.Second = 0;
      RTC.write(tm);
    }
  }
  upPushedPrev = upPushed;
  downPushedPrev = downPushed;
  matrix.clear();
 
  matrix.drawChar( 0, 2, tens+48, savedColor, 0x0000,1);
  matrix.drawChar( 6, 2, units+48, savedColor, 0x0000,1);
  matrix.show();
}

void adjustBrightness(int upPushed, int downPushed)
//void adjustBrightness(int selectPushed)
{
  if(!upPushed)
  //if(selectPushed == 48)
  {
    if(upPushed != upPushedPrev)
    {
      brightness = (radius + 1 > 5) ? 255 : (radius + 1) * 255 / 5;
      radius = radius + 1 > 5 ? 5: radius + 1;
    }
    //brightness = (brightness + 20 > 255) ? 255 : brightness + 20;
  }
  else if(!downPushed)
  //else if(selectPushed == 49)
  {
    if(downPushed != downPushedPrev)
    {
      brightness = (radius - 1 < 1) ? 0 : (radius - 1) * 255 / 5;
      radius = radius - 1 < 0 ? 0 : radius - 1;
    }
    //brightness = (brightness - 20 < 0) ? 0 : brightness - 20;
    
  }
  upPushedPrev = upPushed;
  downPushedPrev = downPushed;
  matrix.clear();
  matrix.drawFastVLine(5,5-radius,2*radius+1,savedColor);
  matrix.drawFastHLine(5-radius,5,2*radius+1,savedColor);
  matrix.drawLine(5-radius,5-radius,5+radius,5+radius,savedColor);
  matrix.drawLine(5-radius,5+radius,5+radius,5-radius,savedColor);
  matrix.setBrightness(brightness);
  matrix.show();
  
}

void adjustColor(int upPushed, int downPushed)
//void adjustColor(int selectPushed)
{

  if(!upPushed && downPushed)
  //if(selectPushed == 48)
  {
    if(upPushed != upPushedPrev)
    {
      colorPos = (colorPos + 1 > 27) ? 0 : colorPos + 1;
      savedColor = Wheel(((colorPos+1) * 256/ 28) % 256);
    }
  }
  else if(!downPushed && upPushed)
  //else if(selectPushed == 49)
  {
    if(downPushed != downPushedPrev)
    {
      colorPos = (colorPos - 1 < 0) ? 27 : colorPos - 1 ;
      savedColor = Wheel(((colorPos+1) * 256/ 28) % 256);
    }
  }
  else if(!upPushed && !downPushed)
  {
    if(upPushed != upPushedPrev)
    {
      if(downPushed != downPushedPrev)
        savedColor = 0xffff;
    }
  }
  upPushedPrev = upPushed;
  downPushedPrev = downPushed;
  
  
  
  wordColor = savedColor;
  matrix.clear();
  matrix.drawCircle(5,5,5,savedColor);
  matrix.drawPixel(colorWheel[colorPos][0],colorWheel[colorPos][1],0xffff);
  matrix.show();
  
}

void showDigitMap(tmElements_t tm) 
{
  uint16_t units, tens;
  uint16_t tempBrightness;
  tempBrightness = brightness / 3;
   
  if (tm.Second/2 % 2) 
  {
    units = tm.Minute % 10;
    tens  = tm.Minute / 10;
    matrix.setBrightness(tempBrightness);
  } 
  else 
  {
    units = tm.Hour % 10;
    tens  = tm.Hour / 10;
    matrix.setBrightness(brightness);
  }
  matrix.clear();
 
  matrix.drawChar( 0, 2, tens+48, savedColor, 0x0000,1);
  matrix.drawChar( 6, 2, units+48, savedColor, 0x0000,1);
  matrix.show();
}

void birthday()
{
  uint16_t i, j, k;

  for(j=0; j<256; j++) 
  { 
    for(i=0; i < 20; i++) 
    {
        matrix.drawPixel((uint16_t)birthdayLEDS[i][0],(uint16_t)birthdayLEDS[i][1],Wheel(((i * 256 / 20) + j) & 255));
        //strip.setPixelColor(i, );
    }
    matrix.show();
    //delay(30);
  }
}

uint16_t Wheel(uint16_t WheelPos) 
{
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) 
  {
    return matrix.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) 
  {
    WheelPos -= 85;
    return matrix.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return matrix.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}
  
void showWordMap()
{
  for( uint16_t row = 0; row < ROWS; row++)
  {
    for( byte column = 0; column < COLUMNS; column++)
    {
      bool onOff = bitRead(wordMap[row],COLUMNS-column-1);
      //Serial.print(onOff);
      switch(onOff)
      {
        case 0:
              matrix.drawPixel(column,row,0);
              break;
        case 1:
              matrix.drawPixel(column,row,wordColor);
              break;
      }
    }
    //Serial.println();
    
    wordMap[row] = 0;
  }
  //Serial.println("------------------------------------");
  
  matrix.show();
}

void wordTime(tmElements_t tm)
{
  keyITS;
  if (tm.Minute <= 2)
  {
    showHour(tm,0);
    if(tm.Hour != 0 && tm.Hour != 12)
    {
      keyOCLOCK;
    }
  }
  else if(tm.Minute > 2 && tm.Minute <= 7)
  {
    keyFIVE;
    keyPAST;
    showHour(tm,0);
  }
  else if(tm.Minute > 7 && tm.Minute <= 12 )  
  {
    keyTEN;
    keyPAST;
    showHour(tm,0);
  }
  else if(tm.Minute > 12 && tm.Minute <= 17)
  {
    keyA;
    keyQUARTER;
    keyPAST;
    showHour(tm,0);
  }
  else if(tm.Minute > 17 && tm.Minute <= 22)
  {
    keyTWENTY;
    keyPAST;
    showHour(tm,0);
  }
  else if(tm.Minute > 22 && tm.Minute <= 27)
  {
    keyTWENTY;
    keyFIVE;
    keyPAST;
    showHour(tm,0);
  }
  else if(tm.Minute > 27 && tm.Minute <= 32)
  {
    keyHALF;
    keyPAST;
    showHour(tm,0);
  }
  else if(tm.Minute > 32 && tm.Minute <= 37)
  {
    keyTWENTY;    
    keyFIVE;
    keyTIL;
    showHour(tm,1);
  }
  else if(tm.Minute > 37 && tm.Minute <= 42)
  {
    keyTWENTY;
    keyTIL;
    showHour(tm,1);
  }
  else if(tm.Minute > 42 && tm.Minute <= 47)
  {
    keyA;
    keyQUARTER;
    keyTIL;
    showHour(tm,1);
  }
  else if(tm.Minute > 47 && tm.Minute <= 52)
  {
    keyTEN;
    keyTIL;
    showHour(tm,1);
  }
  else if(tm.Minute > 52 && tm.Minute <= 57 )
  {
    keyFIVE;
    keyTIL;
    showHour(tm,1);
  }
  else
  {
    showHour(tm,1);
    if(tm.Hour != 23 && tm.Hour != 11)
    {
      keyOCLOCK;
    }        
  }
}

void showHour(tmElements_t tm, bool when)
{
  if(!when)
  { 
    switch(tm.Hour)
    {
      case 0:
          hourMIDNIGHT;
          break;
      case 1:
      case 13:
          hourONE;
          break;
      case 2:
      case 14:
          hourTWO;
          break;
      case 3:
      case 15:
          hourTHREE;
          break;
      case 4:
      case 16:
          hourFOUR;
          break;
      case 5:
      case 17:
          hourFIVE;
          break;
      case 6:
      case 18:
          hourSIX;
          break;
      case 7:
      case 19:
          hourSEVEN;
          break;
      case 8:
      case 20:
          hourEIGHT;
          break;
      case 9:
      case 21:
          hourNINE;
          break;
      case 10:
      case 22:
          hourTEN;
          break;
      case 11:
      case 23:
          hourELEVEN;
          break;
      case 12:
          hourNOON;
          break;
    }
  }
  else
  {
      switch(tm.Hour)
      {
        case 0:
        case 12:
            hourONE;
            break;
        case 1:
        case 13:
            hourTWO;
            break;
        case 2:
        case 14:
            hourTHREE;
            break;
        case 3:
        case 15:
            hourFOUR;
            break;
        case 4:
        case 16:
            hourFIVE;
            break;
        case 5:
        case 17:
            hourSIX;
            break;
        case 6:
        case 18:
            hourSEVEN;
            break;
        case 7:
        case 19:
            hourEIGHT;
            break;
        case 8:
        case 20:
            hourNINE;
            break;
        case 9:
        case 21:
            hourTEN;
            break;
        case 10:
        case 22:
            hourELEVEN;
            break;
        case 11:
            hourNOON;
            break;
        case 23:
            hourMIDNIGHT;
            break;
           
       }
  }
}

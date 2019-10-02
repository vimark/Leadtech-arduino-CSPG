
// Timur Maksiomv 2014
//
// A quick demo of how to use DS1302-library to make a quick
// clock using a DS1302 and a 16x2 LCD.
//
// I assume you know how to connect the DS1302 and LCD.
// DS1302:  CE pin    -> Arduino Digital 5
//          I/O pin   -> Arduino Digital 6
//          SCLK pin  -> Arduino Digital 7
//

//#include <LiquidCrystal.h>
#include <DS1302RTC.h>
#include <Time.h>
#include <TimeLib.h>
#include <U8glib.h>

// Init the DS1302
// Set pins:  CE, IO,CLK
DS1302RTC RTC(5, 6, 7);

char strTime[9]; //Lateral time string variable to display
char strDate[15]; //Lateral time string variable to display

//u8glib
//U8GLIB_SSD1306_128X64 u8g(13, 11, 10, 9);  // SW SPI Com: SCK = 13, MOSI = 11, CS = 10, A0 = 9
U8GLIB_SH1106_128X64 u8g(U8G_I2C_OPT_NONE);
// Init the LCD
//   initialize the library with the numbers of the interface pins
//            lcd(RS,  E, d4, d5, d6, d7)
//LiquidCrystal lcd(8,   9,  4,  5,  6,  7);

void draw(void) {
  // graphic commands to redraw the complete screen should be placed here  
  u8g.setFont(u8g_font_unifont);
  //u8g.setFont(u8g_font_osb21);
  u8g.drawStr( 0, 22, "This is CSPG");
}

void draw_str(const char *s) {
  u8g.firstPage();
  do {
    u8g.drawStr( 0, 22, s);
  } while( u8g.nextPage() );
}

void draw_str(const char * strLine1, const char * strLine2) {
  u8g.firstPage();  
  do {
    u8g.drawStr( 0, 22, strLine1);
    u8g.drawStr( 0, 44, strLine2);
  } while( u8g.nextPage() );
}

void draw_str(unsigned int x, unsigned int y, const char *s) {
  u8g.firstPage();  
  do {
    u8g.drawStr( x, y, s);
  } while( u8g.nextPage() );
}

void setup()
{
  pinMode(8, OUTPUT);
  //reset OLED display
  digitalWrite(8, LOW);
  digitalWrite(8, HIGH);
  
  //u8glib setup
  // assign default color value
  if ( u8g.getMode() == U8G_MODE_R3G3B2 ) {
    u8g.setColorIndex(255);     // white
  }
  else if ( u8g.getMode() == U8G_MODE_GRAY2BIT ) {
    u8g.setColorIndex(3);         // max intensity
  }
  else if ( u8g.getMode() == U8G_MODE_BW ) {
    u8g.setColorIndex(1);         // pixel on
  }
  else if ( u8g.getMode() == U8G_MODE_HICOLOR ) {
    u8g.setHiColorByRGB(255,255,255);
  }
  //set font
  u8g.setFont(u8g_font_unifont);

  // Check clock oscillation  
  if (RTC.haltRTC())
    draw_str("Clock stopped!");
  else
    draw_str("Clock working.");
    
  delay(1000);
  
  // Check write-protection
  if (RTC.writeEN())
    draw_str("Write allowed.");
  else
    draw_str("Write protected.");

  delay ( 1000 );

  // Setup Time library
  draw_str("RTC Sync");
  setTime(20,12,00,29,9,2019); //hr,min,sec,day,mnth,yr
  setSyncProvider(RTC.get); // the function to get the time from the RTC
  
  if(timeStatus() == timeSet)
    draw_str(" Ok!");
  else
    draw_str(" FAIL!");

  delay ( 2000 );
}

void loop()
{

  // Display time centered on the upper line
  GetTimeInStr(strTime, hour(), minute(), second());
  GetDateInStr(strDate, weekday(), month(), day(), year());
  draw_str(strTime, strDate);
  
  // Warning!
  /*if(timeStatus() != timeSet) {
    //lcd.setCursor(0, 1);
    //lcd.print(F("RTC ERROR: SYNC!"));
    draw_str("RTC ERROR: SYNC!");
  }*/

  delay ( 1000 ); // Wait approx 1 sec
}

//Format time in a nice string to be displayed
//Parameters: (string container, hour, min, sec)
void GetTimeInStr(char * vString, int vHour, int vMinute, int vSecond){
  char tStr1[3];
  char tStr2[3];

  //Display always in double digit
  if(vHour < 10){
    itoa(0, tStr1, 10);
    itoa(vHour, tStr2, 10);
    strcat(tStr1, tStr2);
  }
  else{
    itoa(vHour, tStr1, 10);
  }
  
  strcat(tStr1, ":");
  
  if(vMinute < 10){
    strcat(tStr1, "0");
    itoa(vMinute, tStr2, 10);
    strcat(tStr1, tStr2);
  }
  else{
    itoa(vMinute, tStr2, 10);
    strcat(tStr1, tStr2);
  }
  
  strcat(tStr1, ":");
  
  if(vSecond < 10){
    strcat(tStr1, "0");
    itoa(vSecond, tStr2, 10);
    strcat(tStr1, tStr2);
  }
  else{
    itoa(vSecond, tStr2, 10);
    strcat(tStr1, tStr2);
  }
  
  strcpy(vString, tStr1);
}

//Format time in a nice string to be displayed
//Parameters: (string container, weekday, month, date, year)
void GetDateInStr(char * vString, int vWeekday, int vMonth, int vDay, int vYear){
  char tStr1[4];
  char tStr2[5];

  switch(vWeekday){
    case 1: strcpy(tStr1, "Sun");
    break;

    case 2: strcpy(tStr1, "Mon");
    break;

    case 3: strcpy(tStr1, "Tue");
    break;

    case 4: strcpy(tStr1, "Wed");
    break;

    case 5: strcpy(tStr1, "Thu");
    break;

    case 6: strcpy(tStr1, "Fri");
    break;

    case 7: strcpy(tStr1, "Sat");
    break;

    default:  strcpy(tStr1, "Err");
  }
  
  strcat(tStr1, "-");
  
  itoa(vMonth, tStr2, 10);
  strcat(tStr1, tStr2);
  
  strcat(tStr1, "-");

  itoa(vDay, tStr2, 10);
  strcat(tStr1, tStr2);
  
  strcat(tStr1, "-");
  
  itoa(vYear, tStr2, 10);
  strcat(tStr1, tStr2);
    
  strcpy(vString, tStr1);
}

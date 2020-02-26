#include <DS1302RTC.h>

#define VERSION "v0.004b" // Software Version
#define DEV_MESSAGE "Roland Kim Andre Solon"
#define BUILD_NUMBER "Build 0001 06/20/19"
#define UID "DEV-0000-0001"
//Leadtech
//Arduino sketch for CSPG customer module
//June 2019
//Author: trosh, kim

//libraries
//DS1302 RTC from (https://playground.arduino.cc/Main/DS1302RTC/)
//OLED graphics lib from U8glib code (http://code.google.com/p/u8glib/)
//Time https://github.com/PaulStoffregen/Time
//Feb 2020, Add current sense and overcurrent shut-off

#include <DS1302RTC.h>
#include <Time.h>
#include <TimeLib.h>
#include <U8glib.h>
#include <SPI.h>
#include <MFRC522.h>
#include "hardware.h"
#include <Streaming.h> // Easy Serial out
#include <SerialCommand.h> //Using serialcommand by Steven Cogswell https://github.com/scogswell/ArduinoSerialCommand
#include <EEPROM.h>

#define TIME_REQUEST  7    // ASCII bell character requests a time sync message 
#define GMT_plus_8 28800 // +8hrs value in seconds added

//debug

//RTC CMOS/RAM locations, partitions
#define RTC_RAM_SIZE 31
#define LEN_TIME_RATE 8
#define TIME_STOP_LOC 0
#define TIME_STOP_SIZE 10
#define TIME_RATE_LOC TIME_STOP_LOC+TIME_STOP_SIZE+1 //additional +1 since a null character is inserted, waste of space!

//EEPROM locations
#define TIME_STOP_EEPROM_LOC 0
#define TIME_RATE_EEPROM_LOC 16

SerialCommand SCmd; //serial command instance

MFRC522::MIFARE_Key key;
constexpr uint8_t RST_PIN = 9;          // Configurable, see typical pin layout above
constexpr uint8_t SS_PIN = 10;         // Configurable, see typical pin layout above
MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance
// Init the DS1302
// Set pins:  CE, IO,CLK
DS1302RTC RTC(5, 6, 7); // RTC Module
//time_t timeLeft = now(); // must be a unix value
//time_t t = now(); // Current time state
//long time_rate= 259200; // time rate per tap in seconds 
//long time_rate = 10; //10 seconds
long time_rate = 3600; //1hr
int load_rate = 15; // amount to deduct from card
int screen_timeout = 30; // Seconds before screen turns off
time_t screen_now = now(); // Seconds while action.
int pin_OUTPUT = 3; // Pin for power output

boolean isActive=false; //flag if unit is powered on or is loaded with credits
boolean screen_sleep = false;
byte uid[] = {0x54, 0x45, 0x53, 0x54, 0x5f, 0x43, 0x41, 0x52, 0x44, 0x31, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
byte meter_identity[]    = {'C', 'S', 'P', 'G', ' ', 'M', 'E', 'T', 'E', 'R', ' ', '0', '0', '0', '9', 0x00 };
char strTime[9]; //Lateral time string variable to display
char strDate[15]; //Lateral time string variable to display
char strLine1[9];
char strLine2[15];
const char one[] = "ONE\n";
const char two[] = "TWO\n";

//String trString1 = "HELLO";
//String trString2 = "WORLD!";

const char trString1[] = "HELLO";
const char trString2[] = "WORLD!";

//current sense pin and variables
const int analogPin1 = PIN_SENSE_CURRENT;
time_t timeCurrentCheckNow;
int checkCurrentEvery = 1;
boolean checkingCurrentNow = false;
int currentValue = 0;
int currentValue_limit = 1500;
int timeOvercurrent = 0;
int timeOvercurrentGracePeroid = 6; //this is dependent to checkCurrentEvery, timeOvercurrentGracePeroid x checkCurrentEvery
boolean overCurrent = false;
boolean isDisplayCurrent = false;
enum faultType {none, overcurrent, other}; //maybe use this for other fault type?
faultType fault = none;

//RFID stuff
byte sector         = 1;
byte blockAddr      = 4;
byte dataBlock[]    = {  // Initial DataBlock array
  0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00  
};

//RTC stuff including its RAM, store config on its RAM
//32 x 8 RAM
uint8_t ramTimeRate[sizeof(long)];
uint8_t CMOS_buffer[RTC_RAM_SIZE]; // RAM Buffer Array for RTC
enum ramData {TIME_END, TIME_RATE};

//u8glib
//U8GLIB_SSD1306_128X64 u8g(18, 20, 26, 24);  // SW SPI Com: SCK = 13, MOSI = 11, CS = 10, A0 = 9
U8GLIB_SH1106_128X64 u8g(U8G_I2C_OPT_NONE);

//Using U8g2 library
//U8G2_SH1106_128X64_NONAME_1_HW_I2C u8g(U8G2_R0, DISPLAY_CLK, DISPLAY_SDA);

/*
U8G2_SH1106_128X64_NONAME_1_HW_I2C(rotation, [reset [, clock, data]]) [page buffer, size = 128 bytes]
U8G2_SH1106_128X64_NONAME_2_HW_I2C(rotation, [reset [, clock, data]]) [page buffer, size = 256 bytes]
U8G2_SH1106_128X64_NONAME_F_HW_I2C(rotation, [reset [, clock, data]]) [full framebuffer, size = 1024 bytes]
 */

//buzzer setup
const int buzzer = 4; //buzzer to arduino pin 4

void checkCurrent();
void handleOvercurrent();
void yield(){
  
}

void setup()
{
  pinMode(8, OUTPUT);
  pinMode(pin_OUTPUT, OUTPUT);
  digitalWrite(pin_OUTPUT, HIGH);
  pinMode(PIN_WAKE, INPUT_PULLUP);
  Serial.begin(115200);
  Serial.println("[SYS][BOOT]CPSG System");
  Serial.println(BUILD_NUMBER);
  Serial.println(VERSION);
  Serial.println(DEV_MESSAGE);
  Serial.println(UID);

  u8g.begin();
  
  //reset OLED display
  digitalWrite(22, LOW);
  digitalWrite(22, HIGH);
  draw_str("Booting");
  
  //u8glib setup
  // assign default color value
  
  /*if ( u8g.getMode() == U8G_MODE_R3G3B2 ) {
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
  }*/
  //set font
  u8g.setFont(u8g_font_unifont);

  // Check clock oscillation  
  if (RTC.haltRTC()){
    Serial << "\n[SYS][BOOT]RTC has been reset!";

    // Setup Time library
    Serial << "\n[SYS][BOOT]Setting RTC!";
    setTime(12,9,0,5,10,2019); //hr,min,sec,day,mnth,yr
  }
  else Serial << "\n[SYS][BOOT]RTC is running";

  setSyncProvider(RTC.get); // the function to get the time from the RTC
  
  if(timeStatus() == timeSet) Serial << "\n[SYS][BOOT]RTC Synced";
  else Serial << "\n[SYS][BOOT]RTC Warning!";

  SPI.begin();      // Init SPI bus
  mfrc522.PCD_Init();   // Init MFRC522
  #ifdef DEBUG_MODE
  Serial.print("\nRFID Reader ");
  mfrc522.PCD_DumpVersionToSerial();  // Show details of PCD - MFRC522 Card Reader details
  #endif
    // Prepare the key (used both as key A and as key B)
    // using FFFFFFFFFFFFh which is the default at chip delivery from the factory
    for (byte i = 0; i < 6; i++) {
        key.keyByte[i] = 0xFF;
    }
  #ifdef DEBUG_MODE
  dump_byte_array(key.keyByte, MFRC522::MF_KEY_SIZE);
  #endif
  
  //Recover time out data from EEPROM
  checkRemainingTimeAndPowerUp();
  
  screen_now = now() + screen_timeout;
  Serial << "\n[SYS][BOOT] Ready.";
  
  //buzzer
  pinMode(buzzer, OUTPUT); // Set buzzer - pin 4 as an output

  timeCurrentCheckNow = now() + checkCurrentEvery;

  //serial commands
  SCmd.addCommand("DUMP_CMOS", CMD_dump_cmos);
  SCmd.addCommand("SET_RATE", CMD_set_rate);
  SCmd.addCommand("RATE", CMD_display_rate);
  SCmd.addCommand("TIME", CMD_set_time);
  SCmd.addCommand("TIME_RTC", CMD_time_rtc);
  SCmd.addCommand("TIMEOUT", CMD_display_timeout);
  SCmd.addCommand("TEST", CMD_test_function); //for test only
  SCmd.addCommand("CLEAR_TIMEOUT", CMD_clear_timeout);
  SCmd.addCommand("DUMP_EEPROM", CMD_dump_eeprom);
  SCmd.addCommand("CURRENT", CMD_current);
  SCmd.addDefaultHandler(unrecognized);

}

void loop()
{
  handleTimeout();
  handleCurrentCheck();

  SCmd.readSerial(); //serial command handler
  
  if(digitalRead(PIN_WAKE) == LOW){// wake button handler
    Serial.println("[SYS][UI] Wake Pressed");
    if(fault==overcurrent){
      checkRemainingTimeAndPowerUp();
      fault = none;
    }
    else wakeScreen();
  }
  
  // Current Time Handler
  if(isActive==false){
    OLED_displayTime(); //if not loaded, idling
    isActive=false; //weird! the status is already false, but when this line is omitted it turns active status to true
  }
  else{
    if(isTimeUp()) powerOff();
    else OLED_displayRemainingTime();
  }
  
  //check current every 10 seconds
  //store time now
  //check if elapsed time is 5 seconds
  //if time elapsed is 5 seconds, read current
  //if current exceeds current limit, tick current flag, wait for 10 seconds 
  //if in 10 seconds current limit flag is still up, power off output and flash Limit current on screen display

  if(checkingCurrentNow == true) checkCurrent();
  
  //Read RFID
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return;
  }
  
  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    return;
  }else {
    wakeScreen();
    handleReadRFID(); // card read handler
  }
  
}

void powerOff(){
      isActive=false;
      Serial << "\n[SYS][OUTPUT] Power deactivated.\n";
      beep_no_credit();
      digitalWrite(pin_OUTPUT, HIGH);
      wakeScreen();
      draw_str("Time Expired.");
      delay(1000);
      draw_str("Power OFF");
      delay(2000);
//      wakeScreen();
}

void powerOff_overcurrent(){
      isActive=false;
      Serial << "\n[SYS][OUTPUT] Overload! Power deactivated.\n";
      beep_no_credit();
      digitalWrite(pin_OUTPUT, HIGH);
      wakeScreen();
      draw_str("Overload!");
      delay(5000);
      draw_str("Power OFF");
      delay(2000);
//      wakeScreen();
}

void OLED_displayTime(){
  
  GetTimeInStr(strTime, hourFormat12(), minute(), second(), isAM());
  GetDateInStr(strDate, weekday(), month(), day(), year());
  draw_str(strTime, dayStr(weekday()), strDate);
}

void OLED_displayRemainingTime(){

  time_t t = get_time_stop();
  
  long days =  ((t - now())/60/60/24);
  long hours = ((t - now())/60/60 - days * 24);
  long minutes ((t - now())/60 - hours*60 - days * 60 * 24 );
  long seconds = ((((t - now()) - minutes*60) - hours * 60 * 60)  - days * 60 * 60 * 24);

 convertString(strLine1, days );
 GetTimeInStr(strLine2, hours, minutes, seconds); //need to fix this

 //test
 //draw_str(trString1, trString2);
 draw_str(strLine1, strLine2);
}

//Utils
// OLED Screen Section

void wakeScreen() { //Screen Waker
    screen_sleep = false;
    screen_now = now() + screen_timeout;
    u8g.sleepOff();
}

void handleTimeout(){
//  Serial << "\n" << screen_now - now();
  if(screen_sleep == false){ // screen timeout handler
    if(  screen_now - now() <= 0){
      Serial << "\n[SYS][OSD] Turning Screen Off...\n";
      u8g.sleepOn();
      screen_sleep = true;
    }
  }
}

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

  //String str1 = "Hello";
  //String str2 = "World!";
  do {
    u8g.drawStr( 0, 22, strLine1);
    u8g.drawStr( 0, 44, strLine2);
    //u8g.drawStr( 0, 22, "HELLO");
    //u8g.drawStr( 0, 44, "WORLD!");
    //u8g.drawStr( 0, 22, *str1);
    //u8g.drawStr( 0, 44, *str2);
  } while( u8g.nextPage() );
}

/*void draw_str(String *strLine1, String *strLine2) {
  u8g.firstPage();  
  do {
    u8g.drawStr( 0, 22, strLine1);
    u8g.drawStr( 0, 44, strLine2);
  } while( u8g.nextPage() );
}*/

void draw_str(const char * strLine1, const char * strLine2, const char * strLine3) {
  u8g.firstPage();  
  do {
    u8g.drawStr( 0, 22, strLine1);
    u8g.drawStr( 0, 38, strLine2);
    u8g.drawStr( 0, 54, strLine3);
  } while( u8g.nextPage() );
}

void draw_str(unsigned int x, unsigned int y, const char *s) {
  u8g.firstPage();  
  do {
    u8g.drawStr( x, y, s);
  } while( u8g.nextPage() );
}

//RFID Section and its handlers
void handleReadRFID() {

  draw_str("[RFID]Reading IDCARD...");
  
  byte trailerBlock   = 7;
    MFRC522::StatusCode status;
    byte buffer[18];
    byte size = sizeof(buffer);
    
  status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("PCD_Authenticate() failed: "));
        draw_str("RFID Rejected");
#ifdef DEBUG_MODE
        Serial.println(mfrc522.GetStatusCodeName(status));
#endif
        return;
    }

    //read block 5 for UID verification
#ifdef DEBUG_MODE
    Serial << "\n[SYS][RFID] Reading Auth Data\n";
#endif
     status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(5, buffer, &size);
    if (status != MFRC522::STATUS_OK) {
        draw_str("RFID Data Error");
        Serial.print(F("MIFARE_Read() failed: "));
#ifdef DEBUG_MODE
        Serial.println(mfrc522.GetStatusCodeName(status));
#endif
    }
    String load_auth = dump_byte_array(buffer, 16);
#ifdef DEBUG_MODE
    Serial << "\n" <<load_auth <<"\n";
#endif
    for(int i=0;i<16;i++){
      if(load_auth[i] != uid[i]){
        draw_str("Invalid RFID");
        Serial << "[SYS]Invalid RFID UID";
        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1();
        delay(2000);
        return;
      }
    }

    //read block 6, check card association to which meter
#ifdef DEBUG_MODE
    Serial << "\n[SYS][RFID] Reading Auth Data\n";
#endif
     status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(6, buffer, &size);
    if (status != MFRC522::STATUS_OK) {
        draw_str("RFID Data Error");
        Serial.print(F("MIFARE_Read() failed: "));
#ifdef DEBUG_MODE
        Serial.println(mfrc522.GetStatusCodeName(status));
#endif
    }
    load_auth = dump_byte_array(buffer, 16);
#ifdef DEBUG_MODE
    Serial << "\n" <<load_auth <<"\n";
#endif
    for(int i=0;i<16;i++){
      if(load_auth[i] != meter_identity[i]){
        draw_str("Invalid RFID");
        Serial << "[SYS]Invalid RFID UID";
        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1();
        delay(2000);
        return;
      }
    }
    
    status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, buffer, &size);
    if (status != MFRC522::STATUS_OK) {
        draw_str("RFID Data Error");
        Serial.print(F("MIFARE_Read() failed: "));
#ifdef DEBUG_MODE
        Serial.println(mfrc522.GetStatusCodeName(status));
#endif
    }
    beep_buzzer();
    //Below code is for verified cards
    String load = dump_byte_array(buffer, 16);
    int load_int = load.toInt();
    if(load_int - load_rate >= 0){
      int x = load_int - load_rate;
      String y = (String)x;
#ifdef DEBUG_MODE
      Serial << "\n[SYS][RFID] Current Load: " << load << "\n";
      Serial << "[SYS][RFID] Remaining Load: " << y << "\n";
#endif
      byte tempBuffer[y.length()];
      for(int m=0;m<y.length();m++){ //copy string to buffer
        tempBuffer[m] = y[m];
      }
#ifdef DEBUG_MODE
      for(int m=0;m<y.length();m++){ //can erase.
        Serial.println((int)tempBuffer[m]);
      }
#endif
      for(int m=0;m<16;m++) // initialize dataBlock array
        dataBlock[m]=0x00;
      for(int m=0;m<y.length();m++){ // write to dataBlock array
        dataBlock[m] =y[m];
      }
      status = (MFRC522::StatusCode) mfrc522.MIFARE_Write(blockAddr, dataBlock, 16); // Write to RFID
       if (status != MFRC522::STATUS_OK) {
        Serial.print(F("MIFARE_Write() failed: "));
#ifdef DEBUG_MODE
        Serial.println(mfrc522.GetStatusCodeName(status));
#endif
        draw_str("Tap card Error.");
        delay(2000);
      }else{
      String temp = "Bal.: ";
      temp += y;
      String line2 = "Loaded!";
      char screenBuffer[20]; 
      char screenBuffer2[20];
      temp.toCharArray(screenBuffer2, temp.length()+1);
      line2.toCharArray(screenBuffer,line2.length()+1);
      draw_str(screenBuffer2, screenBuffer);
      handleActive();
      updateTimeToStop();
      delay(2000);  
//      wakeScreen();
      }
      
    }else{
      char strtemp[5];
      strcpy(strtemp, "Load");
      draw_str("Insufficient", strtemp);
#ifdef DEBUG_MODE
      Serial << "\n[SYS][RFID] Insufficient Load \n" << "Load Left: " << load_int << "\n";
#endif
      delay(1000);
    }

    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();

    checkingCurrentNow = true; //resume current checking
}

//Power Handler Section
void handleActive() {
  
  if(isActive==false){
    isActive = true;
    digitalWrite(pin_OUTPUT,LOW); 
  }

  Serial.println("\n[SYS][OUTPUT]Power Active!");
}

void updateTimeToStop(){

  time_t tStop = get_time_stop();
  long tRate = get_time_rate();
  
  if(tStop < now()) tStop = now() + tRate;
  else tStop += tRate;
  
  save_time_stop_eeprom(tStop);
}

//Miscellaneous Utility Functions
//Format time in a nice string to be displayed
//Parameters: (string container, hour, min, sec)
void GetTimeInStr(char * vString, int vHour, int vMinute, int vSecond){
  char tStr1[5];
  char tStr2[5];

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

void GetTimeInStr2(String vString, int vHour, int vMinute, int vSecond){
  String str = " ";
  char tStr1[5];
  char tStr2[5];

  //Display always in double digit
  if(vHour < 10){
    //itoa(0, tStr1, 10);
    str += '0';
    //itoa(vHour, tStr2, 10);
    str += vHour;
    //strcat(tStr1, tStr2);
  }
  else{
    //itoa(vHour, tStr1, 10);
    str +=vHour;
  }
  
  //strcat(tStr1, ":");
  str += ':';
  
  if(vMinute < 10){
    //strcat(tStr1, "0");
    str += '0';
    //itoa(vMinute, tStr2, 10);
    //strcat(tStr1, tStr2);
    str += vMinute;
  }
  else{
    //itoa(vMinute, tStr2, 10);
    //strcat(tStr1, tStr2);
    str += vMinute;
  }
  
  //strcat(tStr1, ":");
  str += ':';
  
  if(vSecond < 10){
    //strcat(tStr1, "0");
    str += '0';
    //itoa(vSecond, tStr2, 10);
    //strcat(tStr1, tStr2);
    str += vSecond;
  }
  else{
    //itoa(vSecond, tStr2, 10);
    //strcat(tStr1, tStr2);
    str += vSecond;
  }

  //strcpy(vString, tStr1);
  vString = str;
}

void GetTimeInStr(char * vString, int vHour, int vMinute, int vSecond, bool am_pm){
  char tStr1[5];
  char tStr2[5];

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

  if(am_pm) strcat(tStr1, " AM");
  else strcat(tStr1, " PM");
  
  strcpy(vString, tStr1);
}

//Format time in a nice string to be displayed
//Parameters: (string container, weekday, month, date, year)
void convertString(char * vString, int vDay){
  char string[30];
  char tStr2[20];

  //String str = "Time Left:";
  
  strcpy(string, "Time Left:");
  if(vDay>0){
    itoa(vDay, tStr2, 10);
    strcat(string, tStr2);
    //str += vDay;
  }else{
    strcat(string, "0");
    //str += '0';
  }
  strcat(string, " days");
  //str += " days";
  strcpy(vString, string);
  //vString = str;
}

void GetDateInStr(char * vString, int vWeekday, int vMonth, int vDay, int vYear){
  char tStr1[4];
  char tStr2[5];
  
  //We're using TimeLib now, use date strings
    
  //strcpy(tStr1, dayShortStr(vWeekday));
  //strcat(tStr1, " ");
  strcpy(tStr1, monthShortStr(vMonth));
  strcat(tStr1, "-");
  itoa(vDay, tStr2, 10);
  strcat(tStr1, tStr2);
  strcat(tStr1, "-");
  itoa(vYear, tStr2, 10);
  strcat(tStr1, tStr2);
  strcpy(vString, tStr1);
}

time_t requestSync()
{
  Serial.write(TIME_REQUEST);  
  return 0; // the time will be sent later in response to serial mesg
}

String dump_byte_array(byte *buffer, byte bufferSize) {
  String myString = String((char*)buffer);
#ifdef DEBUG_MODE
  Serial.println(myString);
#endif
//  char * dest;
//  for (int cnt = 0; cnt < bufferSize; cnt++)
//  {
//    // convert byte to its ascii representation
//    sprintf(&dest[cnt * 2], "%02X", buffer[cnt]);
//  }
#ifdef DEBUG_MODE
    for (byte i = 0; i < bufferSize; i++) {
        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i], HEX);
    }
#endif
    return myString;
}

void bufferDump(const char *msg)
{
  Serial.println(msg);
  for (int i=0; i<31; i++)
  {
    //Serial.print("0x");
    if(CMOS_buffer[i] <= 0xF) Serial.print("0");
    Serial.print(CMOS_buffer[i], HEX);
    Serial.print(" ");
    if(!((i+1) % 8)) Serial.println();
  }
  Serial.println();
  Serial.println("--------------------------------------------------------");
}

void initCMOSBuffer(){
 for(int i=0; i<31;i++){
  CMOS_buffer[i] = 0x00; 
 }
}

void writeBuffer(long timestamp){
  String temp1;
  temp1 += timestamp;
  byte buff[RTC_RAM_SIZE];
  for(int i=0;i<temp1.length();i++){
    buff[i] = (int) temp1[i];
  }
  initCMOSBuffer();
  for(int i=0;i<temp1.length();i++){
    CMOS_buffer[i] = buff[i];
  }
  RTC.writeEN(true);
  RTC.writeRAM(CMOS_buffer);
  RTC.writeEN(false);
  initCMOSBuffer();
  RTC.readRAM(CMOS_buffer);
  bufferDump("Reading Newly input data");
  
}

//RTC RAM/CMOS operation handler
//CMOS refers to RTC RAM!
//names are confusing, try to fix these

void CMOS_write(){
  
  RTC.writeEN(true);
  RTC.writeRAM(CMOS_buffer);
  RTC.writeEN(false);

  //dump CMOS
  initCMOSBuffer();
  RTC.readRAM(CMOS_buffer);
  bufferDump("Reading Newly input data");
}

void CMOS_clear_time_out(){
  
  initCMOSBuffer();
  RTC.readRAM(CMOS_buffer); //get CMOS contents
  for(int i=TIME_STOP_LOC; i<TIME_STOP_SIZE; i++) CMOS_buffer[i] = 0; //clear time out partition
  CMOS_write();
}

void writeToRtcRam(long vTime, ramData vData){
  String temp1;
  temp1 += vTime;
  byte buff[RTC_RAM_SIZE];
  
  if(vData==TIME_END){

    initCMOSBuffer();
    RTC.readRAM(CMOS_buffer); //get CMOS contents
    
    for(int i=0; i<=TIME_STOP_SIZE; i++) CMOS_buffer[i] = (int) temp1[i];
    
    CMOS_write();
  }
  if(vData==TIME_RATE){

    initCMOSBuffer();
    RTC.readRAM(CMOS_buffer); //get CMOS contents
    
    for(int i=TIME_RATE_LOC; i<=LEN_TIME_RATE; i++) CMOS_buffer[i] = 0; //clear time rate partition
    for(int i=0; i<=LEN_TIME_RATE; i++) CMOS_buffer[i+TIME_RATE_LOC] = (int) temp1[i]; //copy contents to partition
    
    CMOS_write();
  }
}

//buzzer stuffs

void beep_buzzer(){
  
  //with self oscillating buzzer
#ifdef USING_SELF_OSC_BUZZER
  digitalWrite(buzzer, HIGH);
  delay(100);        // ...for 0.1 sec
  digitalWrite(buzzer, LOW);
#else
 //for no self oscilling buzzer
  tone(buzzer, 1500); // Send 1KHz sound signal...
  delay(100);        // ...for 0.1 sec
  noTone(buzzer);     // Stop sound...
#endif
}

void beep_no_credit(){

  //with self oscillating buzzer
#ifdef USING_SELF_OSC_BUZZER
  digitalWrite(buzzer, HIGH);
  delay(100);        // ...for 0.1 sec
  digitalWrite(buzzer, LOW);
  delay(100);        // ...for 0.1 sec
  digitalWrite(buzzer, HIGH);
  delay(100);        // ...for 0.1 sec
  digitalWrite(buzzer, LOW);
  delay(100);        // ...for 0.1 sec
  digitalWrite(buzzer, HIGH);
  delay(100);        // ...for 0.1 sec
  digitalWrite(buzzer, LOW);
#else
  //for no self oscilling buzzer
  tone(buzzer, 1500); // Send 1KHz sound signal...
  delay(100);        // ...for 0.1 sec
  noTone(buzzer);     // Stop sound...
  delay(100);        // ...for 0.1 sec
  tone(buzzer, 1500); // Send 1KHz sound signal...
  delay(100);        // ...for 0.1 sec
  noTone(buzzer);     // Stop sound...
  delay(100);        // ...for 0.1 sec
  tone(buzzer, 1500); // Send 1KHz sound signal...
  delay(100);        // ...for 0.1 sec
  noTone(buzzer);     // Stop sound...
#endif
}

void double_beep(){
  
  //with self oscillating buzzer
#ifdef USING_SELF_OSC_BUZZER
  digitalWrite(buzzer, HIGH);
  delay(100);        // ...for 0.1 sec
  digitalWrite(buzzer, LOW);
  delay(100);        // ...for 0.1 sec
  digitalWrite(buzzer, HIGH);
  delay(100);        // ...for 0.1 sec
  digitalWrite(buzzer, LOW);
  delay(100);        // ...for 0.1 sec
#else
  //for non self oscilling buzzer
  tone(buzzer, 1500); // Send 1KHz sound signal...
  delay(100);        // ...for 0.1 sec
  noTone(buzzer);     // Stop sound...
  delay(100);        // ...for 0.1 sec
  tone(buzzer, 1500); // Send 1KHz sound signal...
  delay(100);        // ...for 0.1 sec
  noTone(buzzer);     // Stop sound...
  delay(100);        // ...for 0.1 sec
#endif
}

//current check
void handleCurrentCheck(){

  if(timeCurrentCheckNow - now() <= 0){
    currentValue = map(analogRead(analogPin1), ADC_MIN, ADC_MAX, Vmin, Vmax);
    if(isDisplayCurrent) Serial << "\n Current value is:" << " " << currentValue <<"mA";
    if(currentValue >= currentValue_limit){
      double_beep();
      if(overCurrent==false){ //mark as overcurrent and start timing it
        timeOvercurrent = timeOvercurrentGracePeroid;
        overCurrent = true;
      }
      else{
        timeOvercurrent--;
        if(timeOvercurrent <= 0) handleOvercurrent(); //overcurrent timer overflow, declare overloaded
      }
    }
    else overCurrent = false; //just a surge current, dismiss overcurrent flag
    
    checkingCurrentNow = true;
  }
}

void checkCurrent() {
  
  checkingCurrentNow = false;
  timeCurrentCheckNow = now() + checkCurrentEvery;
}

void handleOvercurrent(){
  powerOff_overcurrent();
  overCurrent = false;
  fault = overcurrent;
}

void handleOvercurrentWarning(){
  wakeScreen();
  draw_str("Time Expired.");
  delay(1000);
}

bool isTimeUp(){
  
  time_t t = get_time_stop();
  if(t > now()) return false;
  else return true;
}

void checkRemainingTimeAndPowerUp(){

  time_t t = get_time_stop();
  
  if(t > now()){
        
    Serial << "\nResuming Timer Operations";
    handleActive();
  } 
  else{
    
    Serial << "\nTimer already has expired. Ignoring.";
    isActive = false;
  }
}

//serial commands
void CMD_dump_cmos(){

  Serial.println();
  RTC.readRAM(CMOS_buffer);
  bufferDump("Dumping RTC RAM"); 
}

void CMD_set_rate(){

  char *arg;
  long lBuffer;

  arg = SCmd.next();
  if(arg != NULL){
    lBuffer = atol(arg); //convert string to long int
    time_rate = lBuffer;
    Serial.print("\nTime rate changed to: ");
    Serial.println(lBuffer);

    Serial.print("\nSaving new time rate to CMOS");
    writeToRtcRam(time_rate, TIME_RATE);
    save_time_rate_eeprom(lBuffer);
  }
}

void CMD_set_time(){

  char *arg;
  unsigned long pctime;
  unsigned long DEFAULT_TIME = 1357041600; // Jan 1 2013

  arg = SCmd.next();
  pctime = atol(arg);
  if(arg != NULL){
    if( pctime >= DEFAULT_TIME){ // check the integer is a valid time (greater than Jan 1 2013)
      
      if(RTC.set(pctime + GMT_plus_8) == 0){
        setTime(RTC.get()); // Sync Arduino clock to the time received on the serial port
        Serial.println("[SYS][CLOCK] Time Set.");
      }
      else{
        Serial << "\n[SYS][WARN]RTC was not set!";
      }
      
    }
    else{
      Serial.println("[SYS][CLOCK] Time input was rejected.");
    }
  }
  else{
    
    GetDateInStr(strDate, weekday(), month(), day(), year());
    GetTimeInStr(strTime, hourFormat12(), minute(), second(), isAM());
    Serial.print("\n");
    Serial.print("System time is: ");
    Serial.print(dayStr(weekday()));
    Serial.print(" ");
    Serial.print(strDate);
    Serial.print(" ");
    Serial.print(strTime);
  }  
}

void CMD_test_function(){

  char *arg;
  int i;

  arg = SCmd.next();
  i = atoi(arg);
  Serial.println(arg);
  if(i==1) Serial.println("entered 1");
  if(i==2) Serial.println("entered 2");
}

void CMD_time_rtc(){
  
  time_t t = RTC.get();

  Serial.print("\nRTC time is: ");
  Serial.print(dayShortStr(weekday(t)));  Serial.print(" "); Serial.print(monthShortStr(month(t)));  Serial.print(" "); Serial.print(day(t), DEC);  Serial.print(","); Serial.print(year(t), DEC);
  Serial.print(" - "); Serial.print(hourFormat12(t), DEC); Serial.print(":"); Serial.print(minute(t), DEC); Serial.print(":"); Serial.print(second(t), DEC);
  if(isAM(t))  Serial.print(" AM");
  else  Serial.print(" PM");
}

long get_time_rate(){
  //fetch time rate value from eeprom
  long rate;
  EEPROM.get(TIME_RATE_EEPROM_LOC, rate);
  return rate;
}

void CMD_display_rate(){

  long rate = get_time_rate();
  
  Serial.print("\nRate per tap is: "); Serial.print(rate, DEC);
}

void save_time_rate_eeprom(long rate){
  //save time rate value to eeprom
  
  EEPROM.put(TIME_RATE_EEPROM_LOC, rate);
}

void save_time_stop_eeprom(time_t t){
  
  EEPROM.put(TIME_STOP_EEPROM_LOC, t);
}

time_t get_time_stop(){

  time_t t;
  EEPROM.get(TIME_STOP_EEPROM_LOC, t);
  return t;
}

void unrecognized(){
  
  Serial.println("\nInvalid command!");
}

void CMD_clear_timeout(){

  Serial.println("\nClearing time out entry and powering off");
  
  save_time_stop_eeprom(0);
  powerOff();
}

void CMD_display_timeout(){

  time_t t = get_time_stop();
  
  Serial.print("\nTimeout is on ");
  Serial.print(dayShortStr(weekday(t)));  Serial.print(" "); Serial.print(monthShortStr(month(t)));  Serial.print(" "); Serial.print(day(t), DEC);  Serial.print(","); Serial.print(year(t), DEC);
  Serial.print(" - "); Serial.print(hourFormat12(t), DEC); Serial.print(":"); Serial.print(minute(t), DEC); Serial.print(":"); Serial.print(second(t), DEC);
  if(isAM(t))  Serial.print(" AM");
  else  Serial.print(" PM");
}

void CMD_dump_eeprom(){

  byte value;

  Serial.println("\nDumping EEPROM");
  for (int addr=0; addr<EEPROM.length(); addr++){
    
    value = EEPROM.read(addr);

    if(value <= 0xF) Serial.print("0");
    Serial.print(value, HEX);
    Serial.print(" ");
    if(!((addr+1) % 8)){ Serial.print(addr, DEC); Serial.println();}
  }
  Serial.println();
  Serial.println("--------------------------------------------------------");
}

void CMD_current(){

  char *arg;
  int number;

  arg = SCmd.next();
  number = atoi(arg);
  
  if(arg != NULL){
    if(number == 1){
      Serial.println("\nDisplaying current");
      isDisplayCurrent = true;
    }
    else if(number == 0){
      Serial.println("\nCurrent off");
      isDisplayCurrent = false;
    }
  }
}

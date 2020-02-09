#include <Streaming.h> // Easy Serial out

#define VERSION "v0.004b" // Software Version
#define DEV_MESSAGE "Roland Kim Andre Solon"
#define BUILD_NUMBER "Build 0001 06/20/19"
#define UID "DEV-0000-0001"
//Leadtech
//Arduino sketch for CSPG customer module
//June 2019
//Author: trosh, kim

//snippets used from
//DS1302 RTC from (https://playground.arduino.cc/Main/DS1302RTC/)
//OLED graphics lib from U8glib code (http://code.google.com/p/u8glib/)
//Feb 2020, Add current sense and overcurrent shut-off

#include <DS1302RTC.h>
#include <Time.h>
#include <TimeLib.h>
#include <U8glib.h>
#include <SPI.h>
#include <MFRC522.h>
#include "hardware.h"

#define TIME_HEADER  "T"   // Header tag for serial time sync message
#define TIME_REQUEST  7    // ASCII bell character requests a time sync message 
#define GMT_plus_8 28800 // +8hrs value in seconds added

MFRC522::MIFARE_Key key;
constexpr uint8_t RST_PIN = 9;          // Configurable, see typical pin layout above
constexpr uint8_t SS_PIN = 10;         // Configurable, see typical pin layout above
MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance
// Init the DS1302
// Set pins:  CE, IO,CLK
DS1302RTC RTC(5, 6, 7); // RTC Module
time_t timeLeft = now(); // must be a unix value
time_t t = now(); // Current time state
//long time_rate= 259200; // time rate per tap in seconds 
//long time_rate = 10; //10 seconds
long time_rate = 3600; //1hr
int load_rate = 10; // Load to decrease from card
int screen_timeout = 30; // Seconds before screen turns off
time_t screen_now = now(); // Seconds while action.
int pin_OUTPUT = 3; // Pin for power output
int pin_WAKE = 2; // Pin for Wake
boolean active=false;
boolean screen_sleep = false;
byte uid[] = {0x54, 0x45, 0x53, 0x54, 0x5f, 0x43, 0x41, 0x52, 0x44, 0x31, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
char strTime[9]; //Lateral time string variable to display
char strDate[15]; //Lateral time string variable to display
char strLine1[9];
char strLine2[15];

//current sense pin and variables
const int analogPin1 = PIN_SENSE_CURRENT;
time_t timeCurrentCheckNow;
int checkCurrentEvery = 5;
boolean checkingCurrentNow = false;
int currentValue = 0;
int currentValue_limit = 1500;
int timeOvercurrent = 0;
int timeOvercurrentGracePeroid = 6; //this is dependent to checkCurrentEvery, timeOvercurrentGracePeroid x checkCurrentEvery
boolean overCurrent = false;
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

uint8_t ramBuffer[31]; // RAM Buffer Array for RTC

//u8glib
//U8GLIB_SSD1306_128X64 u8g(18, 20, 26, 24);  // SW SPI Com: SCK = 13, MOSI = 11, CS = 10, A0 = 9
U8GLIB_SH1106_128X64 u8g(U8G_I2C_OPT_NONE);

//buzzer setup
const int buzzer = 4; //buzzer to arduino pin 4

void checkCurrent();
void handleOvercurrent();

void setup()
{
  pinMode(8, OUTPUT);
  pinMode(pin_OUTPUT, OUTPUT);
  digitalWrite(pin_OUTPUT, HIGH);
  pinMode(pin_WAKE, INPUT_PULLUP);
  Serial.begin(115200);
  Serial.println("[SYS][BOOT]CPSG System");
  Serial.println(BUILD_NUMBER);
  Serial.println(VERSION);
  Serial.println(DEV_MESSAGE);
  Serial.println(UID);
  //reset OLED display
  Serial << "\nRate:" << time_rate << "\n";
  digitalWrite(22, LOW);
  digitalWrite(22, HIGH);
  draw_str("Booting");
//  readRAM( uint8_t *p);
//  writeRAM(uint8_t *p);
  
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
  Serial << "[SYS][BOOT]RTC has been reset!";
  else
  Serial << "[SYS][BOOT]RTC is good";
//    draw_str("RTC is running.");
    
//  delay(1000);
  
  // Check write-protection
//  if (RTC.writeEN())
//    draw_str("Write allowed");
//  else
//    draw_str("Write protected");

//  delay ( 1000 );

  // Setup Time library
//  draw_str("RTC Sync");
  setTime(12,9,0,5,10,2019); //hr,min,sec,day,mnth,yr
  setSyncProvider(RTC.get); // the function to get the time from the RTC
  
  if(timeStatus() == timeSet)
//    draw_str("RTC Synced");
Serial << "[SYS][BOOT]RTC Synced";
  else
  Serial << "[SYS][BOOT]RTC Warning!";
//    draw_str("RTC FAILED");

//  delay ( 1000 );

  SPI.begin();      // Init SPI bus
  mfrc522.PCD_Init();   // Init MFRC522
  mfrc522.PCD_DumpVersionToSerial();  // Show details of PCD - MFRC522 Card Reader details
    // Prepare the key (used both as key A and as key B)
    // using FFFFFFFFFFFFh which is the default at chip delivery from the factory
    for (byte i = 0; i < 6; i++) {
        key.keyByte[i] = 0xFF;
    }
  dump_byte_array(key.keyByte, MFRC522::MF_KEY_SIZE);
  initBuffer();
  if(digitalRead(pin_WAKE)==HIGH){
    RTC.readRAM(ramBuffer);
    bufferDump("Reading CMOS Data...");
    String myString = String((char*)ramBuffer);
    Serial.println(myString);
    if(myString.toInt() > now()){
      Serial << "\nResuming Timer Operations";
      timeLeft = myString.toInt();
      active = true;
      digitalWrite(pin_OUTPUT,LOW);
    } else{
      Serial << "\nTimer already has expired. Ignoring.";
      active = false;
    }
  }
//  else if(digitalRead(pin_WAKE)==LOW){
//   initBuffer();
//   Serial << "[SYS] Clearing CMOS RAM" ;
//   writeBuffer(0);
//  }
  
  
  screen_now = now() + screen_timeout;
  Serial << "\n[SYS][BOOT] Ready.";
  //Add something below here that reads the CMOS RAM to read the remaining time. Most likely a UNIX timestamp, write to time_t timeLeft.

  //buzzer
  pinMode(buzzer, OUTPUT); // Set buzzer - pin 4 as an output

  timeCurrentCheckNow = now() + checkCurrentEvery;

}

void loop()
{
  handleTimeout();
  handleCurrentCheck();
  if (Serial.available()) { // Time syncer via Serial monitor
    Serial.println("[SYS]Serial Monitor Activity");
    processSyncMessage();
  }
  
  if(digitalRead(pin_WAKE) == LOW){// wake button handler
    Serial.println("[SYS][UI] Wake Pressed");
    if(fault==overcurrent){
      checkRemainingTimeAndPowerUp();
      fault = none;
    }
    else wakeScreen();
  }
  
  
  // Current Time Handler
  if(!active){
    GetTimeInStr(strTime, hour(), minute(), second());
  GetDateInStr(strDate, weekday(), month(), day(), year());
  draw_str(strTime, dayStr(weekday()), strDate);
  //Serial << "\n" << strDate << " " << strTime; //Uncomment to check time via serial
  }else{
    
    if(now()<timeLeft){
    drawTime();
    }else if(now()>timeLeft){
      powerOff();
    }
  }

  //check current every 10 seconds
  //store time now
  //check if elapsed time is 5 seconds
  //if time elapsed is 5 seconds, read current
  //if current exceeds current limit, tick current flag, wait for 10 seconds 
  //if in 10 seconds current limit flag is still up, power off output and flash Limit current on screen display

  //Serial << " " << numberOfSeconds(now());
  if(checkingCurrentNow == true) checkCurrent();
  
  // Warning!
  /*if(timeStatus() != timeSet) {
    //lcd.setCursor(0, 1);
    //lcd.print(F("RTC ERROR: SYNC!"));
    draw_str("RTC ERROR: SYNC!");
  }*/

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
active=false;
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
active=false;
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

void drawTime(){
 long days =  ((timeLeft - now())/60/60/24);
 long hours = ((timeLeft - now())/60/60 - days * 24);
 long minutes ((timeLeft - now())/60 - hours*60 - days * 60 * 24 );
 long seconds = ((((timeLeft - now()) - minutes*60) - hours * 60 * 60)  - days * 60 * 60 * 24);


//    Serial << "\n" << days << ":" << hours << ":" <<minutes <<":" << seconds <<"\n" ;
convertString(strLine1, days );
GetTimeInStr(strLine2, hours, minutes, seconds);
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
  do {
    u8g.drawStr( 0, 22, strLine1);
    u8g.drawStr( 0, 44, strLine2);
  } while( u8g.nextPage() );
}

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
        Serial.println(mfrc522.GetStatusCodeName(status));
        return;
    }
    Serial << "\n[SYS][RFID] Reading Auth Data\n";
     status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(5, buffer, &size);
    if (status != MFRC522::STATUS_OK) {
        draw_str("RFID Data Error");
        Serial.print(F("MIFARE_Read() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
    }
    String load_auth = dump_byte_array(buffer, 16);
    Serial << "\n" <<load_auth <<"\n";
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
    
    status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, buffer, &size);
    if (status != MFRC522::STATUS_OK) {
        draw_str("RFID Data Error");
        Serial.print(F("MIFARE_Read() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
    }
    beep_buzzer();
    //Below code is for verified cards
    String load = dump_byte_array(buffer, 16);
    int load_int = load.toInt();
    if(load_int - load_rate >= 0){
      int x = load_int - load_rate;
      String y = (String)x;
      Serial << "\n[SYS][RFID] Current Load: " << load << "\n";
      Serial << "[SYS][RFID] Remaining Load: " << y << "\n";
      byte tempBuffer[y.length()];
      for(int m=0;m<y.length();m++){ //copy string to buffer
        tempBuffer[m] = y[m];
      }
      for(int m=0;m<y.length();m++){ //can erase.
        Serial.println((int)tempBuffer[m]);
      }
      for(int m=0;m<16;m++) // initialize dataBlock array
        dataBlock[m]=0x00;
      for(int m=0;m<y.length();m++){ // write to dataBlock array
        dataBlock[m] =y[m];
      }
      status = (MFRC522::StatusCode) mfrc522.MIFARE_Write(blockAddr, dataBlock, 16); // Write to RFID
       if (status != MFRC522::STATUS_OK) {
        Serial.print(F("MIFARE_Write() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
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
      delay(2000);  
//      wakeScreen();
      }
      
    }else{
      char strtemp[5];
      strcpy(strtemp, "Load");
      draw_str("Insufficient", strtemp);
      Serial << "\n[SYS][RFID] Insufficient Load \n" << "Load Left: " << load_int << "\n";
      
      delay(1000);
    }

    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
}

//Power Handler Section
void handleActive() {
  initBuffer();
  Serial << "\nTime Rate:" << time_rate << "\n";
  if(!active){
    active = true;
    digitalWrite(pin_OUTPUT,LOW);
    Serial << "\n[SYS][OUTPUT]Power Active!\n";
    
    timeLeft = now() + time_rate;
  }else{
    timeLeft = timeLeft + time_rate;
    Serial << "\n[SYS][OUTPUT]Added\n";
  }
  //TODO: Use the CMOS RAM to store remaining time. Else, shit happens
  writeBuffer(timeLeft);
  Serial << "\n[SYS][EPOCH]=" << timeLeft << "\n";
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

//Format time in a nice string to be displayed
//Parameters: (string container, weekday, month, date, year)
void convertString(char * vString, int vDay){
  char string[30];
  char tStr2[20];
  strcpy(string, "Time Left:");
  if(vDay>0){
        itoa(vDay, tStr2, 10);
  strcat(string, tStr2);  
  }else{
    strcat(string, "0");
  }
  strcat(string, " days");
  strcpy(vString, string);
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

//Process Time via Serial Monitor function
//Should add a verifier for this to avoid cheating.
void processSyncMessage() {
  unsigned long pctime;
  const unsigned long DEFAULT_TIME = 1357041600; // Jan 1 2013

  if(Serial.find(TIME_HEADER)) {
     pctime = Serial.parseInt();
     if( pctime >= DEFAULT_TIME) { // check the integer is a valid time (greater than Jan 1 2013)
       
       
       if(RTC.set(pctime + GMT_plus_8) == 0){
        setTime(RTC.get()); // Sync Arduino clock to the time received on the serial port
        Serial.println("[SYS][CLOCK] Time Set.");
       }
       else{
        Serial << "\n[SYS][WARN]RTC was not set!";
       }
       
     }else{
      Serial.println("[SYS][CLOCK] Time input was rejected.");
     }
  }
}

time_t requestSync()
{
  Serial.write(TIME_REQUEST);  
  return 0; // the time will be sent later in response to serial mesg
}

String dump_byte_array(byte *buffer, byte bufferSize) {
  String myString = String((char*)buffer);
  Serial.println(myString);
//  char * dest;
//  for (int cnt = 0; cnt < bufferSize; cnt++)
//  {
//    // convert byte to its ascii representation
//    sprintf(&dest[cnt * 2], "%02X", buffer[cnt]);
//  }
  
    for (byte i = 0; i < bufferSize; i++) {
        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i], HEX);
    }

    return myString;
}

void bufferDump(const char *msg)
{
  Serial.println(msg);
  for (int i=0; i<31; i++)
  {
    Serial.print("0x");
    if(ramBuffer[i] <= 0xF) Serial.print("0");
    Serial.print(ramBuffer[i], HEX);
    Serial.print(" ");
    if(!((i+1) % 8)) Serial.println();
  }
  Serial.println();
  Serial.println("--------------------------------------------------------");
}

void initBuffer(){
 for(int i=0; i<31;i++){
  ramBuffer[i] = 0x00; 
 }
}

void writeBuffer(long timestamp){
  String temp1;
  temp1 += timestamp;
  byte buff[31];
  for(int i=0;i<temp1.length();i++){
    buff[i] = (int) temp1[i];
  }
  initBuffer();
  for(int i=0;i<temp1.length();i++){
    ramBuffer[i] = buff[i];
  }
  RTC.writeEN(true);
  RTC.writeRAM(ramBuffer);
  RTC.writeEN(false);
  initBuffer();
  RTC.readRAM(ramBuffer);
  bufferDump("Reading Newly input data");
  
}

//buzzer stuffs

void beep_buzzer(){
  //for no self oscilling buzzer
  /*tone(buzzer, 1500); // Send 1KHz sound signal...
  delay(100);        // ...for 0.1 sec
  noTone(buzzer);     // Stop sound...*/
  //with self oscillating buzzer
  digitalWrite(buzzer, HIGH);
  delay(100);        // ...for 0.1 sec
  digitalWrite(buzzer, LOW);
}

void beep_no_credit(){

  //for no self oscilling buzzer
  /*tone(buzzer, 1500); // Send 1KHz sound signal...
  delay(100);        // ...for 0.1 sec
  noTone(buzzer);     // Stop sound...
  delay(100);        // ...for 0.1 sec
  tone(buzzer, 1500); // Send 1KHz sound signal...
  delay(100);        // ...for 0.1 sec
  noTone(buzzer);     // Stop sound...
  delay(100);        // ...for 0.1 sec
  tone(buzzer, 1500); // Send 1KHz sound signal...
  delay(100);        // ...for 0.1 sec
  noTone(buzzer);     // Stop sound...*/

  //with self oscillating buzzer
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
}

void double_beep(){
  //for no self oscilling buzzer
  /*tone(buzzer, 1500); // Send 1KHz sound signal...
  delay(100);        // ...for 0.1 sec
  noTone(buzzer);     // Stop sound...
  delay(100);        // ...for 0.1 sec
  tone(buzzer, 1500); // Send 1KHz sound signal...
  delay(100);        // ...for 0.1 sec
  noTone(buzzer);     // Stop sound...
  delay(100);        // ...for 0.1 sec*/

  //with self oscillating buzzer
  digitalWrite(buzzer, HIGH);
  delay(100);        // ...for 0.1 sec
  digitalWrite(buzzer, LOW);
  delay(100);        // ...for 0.1 sec
  digitalWrite(buzzer, HIGH);
  delay(100);        // ...for 0.1 sec
  digitalWrite(buzzer, LOW);
  delay(100);        // ...for 0.1 sec
}

//current check
void handleCurrentCheck(){

  if(  timeCurrentCheckNow - now() <= 0){
    currentValue = map(analogRead(analogPin1), ADC_MIN, ADC_MAX, Vmin, Vmax);
    Serial << "\n Current value is:" << " " << currentValue <<"mA";
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

void checkRemainingTimeAndPowerUp(){ //recover from overcurrent shutdown, don't mind the name lol
   //these snippets are from above code starting at line 164, maybe make a function for this?
    initBuffer();
  
    RTC.readRAM(ramBuffer);
    bufferDump("Reading CMOS Data...");
    String myString = String((char*)ramBuffer);
    Serial.println(myString);
    if(myString.toInt() > now()){
      Serial << "\nResuming Timer Operations";
      timeLeft = myString.toInt();
      active = true;
      digitalWrite(pin_OUTPUT,LOW);
    } else{
      Serial << "\nTimer already has expired. Ignoring.";
      active = false;
    }
  
}

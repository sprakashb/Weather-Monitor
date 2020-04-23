//***************************************************************************
// Simple Weather Station by SP Bhatnagar VU2SPF
// Uses BME 280 for Temperature, Pressure and Humidity (inside)
// and BMP 280 for Temperature and Pressure (Outside)
// Also has a DS1307 RTC with Battery for date / time
// 2.8 " MCUFriend non Touch TFT display with Arduino Mega (uses approx45kB Flash)
// Displays Date, Day, Time, Temperatures ( Inside and Outside),
// Pressure and Humidity along with Max and Min values for all measured parameters
// The data from both sensors is stored on SD card at 10 min interval
// The system connects to serial terminal for 1-Taking dump of data collected,
// which then could be copied and stored on PC in space separated variables,
// 2-deleting the data from SD card for fresh data,
// 3-setting the time and date from Serial terminal.
// If using Arduino Mega 4 wires need to be soldered on back side.
// Details on vu2spf.blogspot.com, email: vu2spf@gmail.com
// ***************************************************************************/
// TFT display for weather station by SPB 30/3/2020 during Lockdown with BME280 T/P/H sens
// Added BMP280 T/P  sens  19/4/2020  -- Lockdown --
//
#include <Wire.h>               //  -- Standard libraries with Arduino
#include <SPI.h>                //  -- Standard libraries with Arduino

#include <Adafruit_Sensor.h>  // Adafruits contributed libraries
#include <Adafruit_BME280.h>  // Temp, Press & Humidity
#include <Adafruit_BMP280.h>  // Only Temp & Press
#include <Adafruit_GFX.h>     // Core graphics library located at adafuit website  https://github.com/adafruit/Adafruit-GFX-Library

#include <MCUFRIEND_kbv.h>    // https://github.com/prenticedavid/MCUFRIEND_kbv

#include <DS1307RTC.h>        // Part of RTC Lib 
#include <Time.h>             //  -- Standard libraries with Arduino
#include "RTClib.h"

//#define USE_SDFAT
#include <SdFat.h>            // Use the SdFat library https://github.com/greiman/SdFat

#include "userdefs.h"
#define SEALEVELPRESSURE_HPA (1013.25)

// Create All instances to be used
Adafruit_BME280 bme; // I2C Address 0x77 (changed by jumper by spb)
Adafruit_BMP280 bmp; // I2C Address 0x76

SdFat SD;                     // Hardware SPI based SD Card
SdFile dataFile;              // sdFile type dataFile on SD card
#define SD_CS     10          // Chip Select pin for SD Card on MCUFriend TFT

RTC_DS1307 rtc;           // our clock is called rtc

MCUFRIEND_kbv tft;

//tmElements_t tm;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

unsigned long delayTime = 10; // 10 min between two data
uint16_t identifier, tftwd, tftht, displ_rotn = 1; // rotn 0 == portrait
float maxT = 0.0, minT = 0.0, maxP = 0.0, minP = 0.0, maxH = 0.0, minH = 0.0;
float maxT_outside = 0.0, minT_outside = 0.0, maxP_outside = 0.0, minP_outside = 0.0; // Outside sensor
float currT, currP, currH, currT_outside, currP_outside;
bool status;
bool rtc_ok, bmp_ok, bme_ok;
unsigned long begintime;
int ret_sbmp;
int hr, mn, sc, dt, mon, yr, prev_date = 0; // temp storage
int utchr, utcmn, utcsc;  // utc
String ampm = "TM";

// Function Prototypes in sequence of appearance
// Main Tab
void displ_date_time();
void print2digits(int number);
void display_weather();

void  cmd_chk();
void dump_file();
void delete_data_file();
void print_help();
void print_help2();
void set_time() ;
int get_number();
void save_data();
void save_header();


void setup()
{
  Serial.begin(115200);   // Serial for messages and control
  while (!Serial) {
    SysCall::yield();
  }
  identifier = tft.readID(); // get display ID.
  tft.begin(identifier);        // setup to use driver
  tftwd = tft.height(); //
  tftht = tft.width();
  tft.setRotation(displ_rotn); // LS

  //If using Arduino Mega and hard wired on back
  pinMode(11, INPUT);   // spb - Set to HiZ. hard wired on MEGA back to 51 MOSI
  pinMode(12, INPUT);   // spb - Set to HiZ, hard wired on MEGA back to 50 MISO
  pinMode(13, INPUT);   // spb - Set to HiZ, hard wired on MEGA back to 52 SCK

  if (!SD.begin(SD_CS, SD_SCK_MHZ(50)))
    SD.initErrorHalt();

  tft.fillScreen(BackColor);    // Background colour for display
  tft.drawFastHLine(0, tftht / 4, tftwd, PartitionLineColor); // First qtr of screen for Clock
  tft.drawFastHLine(0, tftht / 2 + 30, tftwd, PartitionLineColor); // 2nd partition In & Out Temp
  tft.drawFastHLine(0, tftht * 3 / 4 + 15, tftwd, PartitionLineColor); // 3rd partn Pressure/ below is 4th Partition Humidity
  tft.drawFastVLine(tftwd / 2, tftht / 4, tftht, PartitionLineColor); // Vertical divider line

  if (rtc.begin())
  {
    rtc_ok = true;
    displ_date_time();
  }   // RTC OK
  else if (!rtc.isrunning())
  {
    Serial.println("RTC is NOT running!");
  }

  status = bme.begin(0x77);
  if (!status)
    bme_ok = false;
  else
    bme_ok = true;

  if (!bmp.begin(0x76))
    bmp_ok = false;
  else
    bmp_ok = true;

  if (bme_ok)
  {
    currT = bme.readTemperature(); // Inside data
    currP = bme.readPressure();
    currH = bme.readHumidity();
    maxT = currT;
    minT = currT;
    maxP = currP;
    minP = currP;
    maxH = currH;
    minH = currH;
  }
  if (bmp_ok)
  {
    currT_outside = bmp.readTemperature(); // Sensor mounted outside
    currP_outside = bmp.readPressure();
    maxT_outside = currT_outside ;
    minT_outside = currT_outside ;
    maxP_outside = currP_outside ;
    minP_outside =  currP_outside ;
  }
  display_weather();
  begintime = millis();
  save_header();
  save_data();  // initial value
}


void loop()
{
  if (Serial.available() >= 1)     // If any command then execute it
    cmd_chk();

  if (bme_ok)         // Inside Temp, Press & Humidity by bme280
  {
    currT = bme.readTemperature();
    currP = bme.readPressure();
    currH = bme.readHumidity();

    if (maxT < currT)  // get max & min
      maxT = currT;
    if (minT > currT)
      minT = currT;

    if (maxP < currP)
      maxP = currP;
    if (minP > currP)
      minP = currP;

    if (maxH < currH)
      maxH = currH;
    if (minH > currH)
      minH = currH;
  }
  if (bmp_ok)      // Outside Temp and Press by BMP 280 sensor
  {
    currT_outside = bmp.readTemperature();
    currP_outside = bmp.readPressure();

    maxT_outside = max(maxT_outside, currT_outside);
    minT_outside = min(minT_outside, currT_outside);
  }

  if (rtc_ok)
    displ_date_time();

  if ((millis() - begintime) > delayTime * 60 * 1000)   // every 5 min
    save_data();  // write to SD Card
}

void displ_date_time()
{
  tft.setTextSize(3);
  tft.setTextColor(TimeColor, BackColor);
  //  tft.setCursor(5, 5);  // Date Time

  if (rtc_ok)
  {
    //  RTC.read(tm);
    tft.setCursor( 5, 5); // Date Time x tftwd / 4

    DateTime now = rtc.now();
    hr = now.hour();
    mn = now.minute();
    sc = now.second();

    utcmn = mn  - utc_min_difference;
    if (utcmn < 0)
    {
      utcmn = utcmn + 60;
      utchr = -1; // 1 hr correction
    }
    else
      utchr = 0;

    utchr = utchr + hr - utc_hr_difference;
    if (utchr < 0 )
      utchr = utchr + 24;

#ifdef AMPM_Fmt
    if (hr < 12)
      ampm = "AM";
    else
      ampm = "PM";
    if (hr == 24 || hr == 12)
      hr = 12;
    else
      hr = hr % 12;
#endif
    print2digits (hr);
    tft.write(':');
    print2digits(mn);
    tft.write(':');
    print2digits(sc);

#ifdef AMPM_Fmt
    //    tft.print(" ");
    tft.setTextSize(2);
    tft.print(ampm);
    tft.print(" ");
#endif

    tft.setTextSize(3);
    print2digits(utchr);
    tft.print(":");
    print2digits(utcmn);
    tft.setTextSize(2);
    tft.print("UTC");


    tft.setCursor(5, 35);
    tft.setTextColor(DayColor, BackColor);
    if (now.dayOfTheWeek() == 3 || now.dayOfTheWeek() == 4 || now.dayOfTheWeek() == 6)
      tft.setTextSize(2);
    else
      tft.setTextSize(3);
    tft.print(daysOfTheWeek[now.dayOfTheWeek()]);
    tft.write(' ');

    tft.setTextSize(3);
    tft.setTextColor(DateColor, BackColor);

    tft.print(now.day());
    tft.write('/');
    tft.print(now.month());
    tft.write('/');
    tft.print(now.year());

    dt = now.day();
    mon = now.month();
    yr = now.year();
  }
  else
  {
    tft.print("VU2SPF Weather Station");
  }
}

void print2digits(int number) {
  if (number >= 0 && number < 10)
  {
    tft.write('0');
  }
  tft.print(number);
}

void display_weather()
{
  if (bme_ok)
  {
    tft.setTextColor(InTempColor, BackColor);
    tft.setTextSize(4);
    tft.setCursor(15, tftht / 4 + 10);  // Inside T
    tft.print(currT, 1); // one decimal digit - Inside T
    tft.setTextSize(2);
    tft.print(char(247));
    tft.setTextSize(4);
    tft.print("C");

    tft.setTextColor(InTempColor, BackColor);
    tft.setCursor(15, tftht / 4 + 45); //char ht~40
    tft.setTextSize(2);
    tft.print("Max:" );
    tft.print(maxT, 1);
    tft.setCursor(tftwd / 2 - 45, tftht / 4 + 50);
    tft.setTextSize(3);
    tft.setTextColor(InTempColor, BackColor);
    tft.print("IN");
    tft.setTextColor(InTempColor, BackColor);
    tft.setTextSize(2);
    tft.setCursor(15, tftht / 4 + 65);
    tft.print("Min:" );
    tft.print(minT, 1);  // + "C");

    tft.setTextSize(2);
    tft.setTextColor(PressColor, BackColor);
    tft.setCursor(10, tftht / 2 + 40);
    tft.setTextSize(3);
    tft.print(currP / 100.0F, 1);
    tft.setTextSize(2);
    tft.print("hPa");
    tft.setCursor(tftwd / 2 + 10, tftht / 2 + 43);
    tft.setTextSize(3);
    tft.print("P");

    tft.setCursor(tftwd / 2 + 35, tftht / 2 + 35);
    tft.setTextSize(2);
    tft.print(maxP / 100.0F, 1);
    tft.print(" Max");
    tft.setTextSize(2);
    tft.setCursor(tftwd / 2 + 35, tftht / 2 + 55);
    tft.print(minP / 100.0F, 1);
    tft.print(" Min");

    tft.setTextColor(HumColor, BackColor);
    tft.setCursor(10, tftht * 3 / 4 + 25);
    tft.setTextSize(3);
    tft.print("H ");
    tft.print(currH, 1);
    tft.print("%");
    tft.setCursor(tftwd / 2 + 5, tftht * 3 / 4 + 20);
    tft.setTextSize(2);
    tft.print(maxH , 1);
    tft.print(" Max");
    tft.setCursor(tftwd / 2 + 5, tftht * 3 / 4 + 40);
    tft.print(minH, 1);
    tft.print(" Min");
  }

  if (bmp_ok)
  {
    tft.setTextColor(OutTempColor, BackColor);
    tft.setTextSize(4);
    tft.setCursor(tftwd / 2 + 15, tftht / 4 + 10);
    tft.print(currT_outside, 1); // one decimal digit
    tft.setTextSize(2);
    tft.print(char(247));
    tft.setTextSize(4);
    tft.print("C");

    tft.setCursor(tftwd / 2 + 5, tftht / 4 + 50); //char ht~40
    tft.setTextSize(3);
    tft.setTextColor(OutTempColor, BackColor);
    tft.print("OUT");
    tft.setCursor(tftwd / 2 + 60, tftht / 4 + 45); //char ht~40
    tft.setTextColor(OutTempColor, BackColor);
    tft.setTextSize(2);
    tft.print("Max:" );
    tft.print(maxT_outside, 1);
    tft.setTextColor(OutTempColor, BackColor);
    tft.setTextSize(2);
    tft.setCursor(tftwd / 2 + 60, tftht / 4 + 65);
    tft.print("Min:" );
    tft.print(minT_outside, 1);
  }
}

void  cmd_chk()   // check and execute serial commands
{
  switch (Serial.read())
  {
    case '\n' : break;
    case 'd': {
        dump_file();
        break;
      }
    case 't': {
        set_time();
        break;
      }
    case 'x': {
        delete_data_file();
        break;
      }
    case 'h': {
        print_help();
        break;
      }
    default : {
        print_help2();
        break;
      }
  }
}

void dump_file()
{
  if ( !dataFile.open("data1.txt",  FILE_READ))
    Serial.println("FIle Open Error @dump");

  dataFile.rewind();

  Serial.println();
  // if the file is available, read it and dump on serial term:
  while (dataFile.available())
  {
    Serial.write(dataFile.read());
  }
  dataFile.close();
  Serial.println();

}

void delete_data_file()
{
  unsigned long entrytime = millis();
  Serial.println();
  Serial.println("please enter y to confirm delete:");
  while (millis() - entrytime < 5000)
  {
    if (Serial.read() == 'y')
      SD.remove("data1.txt");
  }
  Serial.println("Data File Deleted.");
  Serial.println();
  if ( !dataFile.open("data1.txt",  FILE_WRITE))
    Serial.println("FIle Open Error @delete");
  dataFile.close();

  save_header();
  save_data();
  Serial.println("Blank Data File Recreated");
}

void print_help()
{
  Serial.println();
  Serial.println("d-Dump data, x-delete data, t-set time, h-help");
  Serial.println();
}

void print_help2()
{
  Serial.println();
  Serial.println("HELP:d-Dump data, x-delete data, t-set time, h-help");
  Serial.println();
}

void set_time()   // Reads Date and time values from Serial Terminal and loads in RTC
{ int day_t, mon_t, yr_t, hr_t, min_t, sec_t;
  Serial.println("Enter 2 digits for the following in 24 hour format");
  Serial.println("Enter Date (dd)..");
  day_t = get_number();
  Serial.println("Enter Month (mm)..");
  mon_t = get_number();
  Serial.println("Enter year (yy)..");
  yr_t = get_number() + 2000;
  Serial.println("Enter Hour (hh)..");
  hr_t = get_number();
  Serial.println("Enter Min(mm)..");
  min_t = get_number();
  Serial.println("Enter Sec(ss)..");
  sec_t = get_number();

  setTime(hr_t, min_t, sec_t, day_t, mon_t, yr_t);
  RTC.set(now());
  Serial.print("Time / Date Set to :");
  Serial.print(String(hr_t) + ":" + String(min_t) + ":" + String(sec_t) + " ");
  Serial.println(String(day_t) + ":" + String(mon_t) + ":" + String(yr_t));
  displ_date_time();
}


int get_number()   // convt 2 ascii to int
{
  int no = 0;
  char c;
  c = Serial.read();   // skip the CR from previous cmd
  while ((c = Serial.read()) != '\n')
  {
    if ( isDigit(c))
      no = (10 * no) + (c - '0') ; // convert digits to a number
  }
  return no;
}

void save_data()
{
  if ( !dataFile.open("data1.txt", FILE_WRITE | FILE_READ))
    Serial.println("FIle Open Error @save");
  display_weather();
  begintime = millis();
  if (prev_date != dt)   // at date change write header
  {
    dataFile.close();
    save_header();
  }
  //if(mn%10 == 0)  // store data on SDCard on 0,10,20... mins

  // Save data on SD Card
  if (hr < 10)
    dataFile.print("0");
  dataFile.print(hr);
  dataFile.print(":");
  if (mn < 10)
    dataFile.print("0");
  dataFile.print(mn);
  //  dataFile.print(":"); // for printing seconds also
  //  if (sc < 10)
  //    dataFile.print("0");
  //    dataFile.print(sc);

  dataFile.print("  ");
  dataFile.print(currT, 1);
  dataFile.print(" ");
  dataFile.print(currP / 100.0, 1);
  dataFile.print(" ");
  dataFile.print(currH, 1);
  dataFile.print("  ");
  //   dataFile.print("- ");
  dataFile.print(currT_outside, 1);
  dataFile.print(" ");
  dataFile.println(currP_outside / 100.0, 1);
  dataFile.flush();
  dataFile.close();
  prev_date = dt;
}

void save_header()
{
  if ( !dataFile.open("data1.txt", FILE_WRITE | FILE_READ))
    Serial.println("FIle Open Error @header");
  dataFile.print(dt);
  dataFile.print("/");
  dataFile.print(mon);
  dataFile.print("/");
  dataFile.println(yr);
  dataFile.println("-----------------------------------");
  dataFile.println(F("Hr:Min Tin   Pin  Hin   Tout Pout")); // Header
  dataFile.println("-----------------------------------");
  dataFile.close();
}

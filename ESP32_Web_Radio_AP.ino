//  Simple ESP32 web radio player.
//	Based on Nick Koumaris code - very nice job, Nick!
//  https://www.instructables.com/Internet-Radio-Using-an-ESP32/
//  https://github.com/educ8s/ESP32-Web-Radio-Simple

//  Customized by Marc Stähli / Jan 2021
//  - added rotary knob to change channels
//  - added 128x32 oled display
//  - stereo version
//  - added WiFiManger to config WiFi credentials over an access point
//  - added header file Stations.h for easier stations management
//  Modified in Juli 2022
//  - Wifi Manager change
//  - corrected stations list
//  - refactored code with ESP32_VS1053_Stream Library
//  - adding Metadata such as song information

#include <VS1053.h>               //https://github.com/baldram/ESP_VS1053_Library
#include <ESP32_VS1053_Stream.h>  //https://github.com/CelliesProjects/ESP32_VS1053_Stream
#include <ESP32Encoder.h>         //https://github.com/madhephaestus/ESP32Encoder
#include <HTTPClient.h>
#include <EEPROM.h>
#include <U8g2lib.h>              //https://github.com/olikraus/u8g2
#include <Wire.h>
#include <esp_wifi.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include "Stations.h"             // Radio station settings

#define VS1053_CS    32
#define VS1053_DCS   33
#define VS1053_DREQ  35

#define VOLUME       95         // volume level 0-100
#define EEPROM_SIZE  64

ESP32_VS1053_Stream stream;

//***** RotaryEncoder
const int rotaryDT  = 16;
const int rotaryCLK = 17;
const int rotarySW  = 26;          // Switch is used to trigger a reset when rotary is pushed
ESP32Encoder encoder;

//***** OLED
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

long interval = 1000;
int SECONDS_TO_AUTOSAVE = 30;
long seconds = 0;
long previousMillis = 0;

int radioStation = 0;
int previousRadioStation = -1;
String songinfo;
int shift = 128;
int textwidth;
unsigned int currentState = 0;  // variable for statemachine in loop

void setup () {

  Serial.begin(115200); while (!Serial); delay(200);
  Serial.println("Starting up Web Radio");
  delay(500);

  SPI.begin();

  u8g2.begin();                           // initialize Screen
  u8g2.clearBuffer();                     // clear the internal memory
  u8g2.setFont(u8g2_font_t0_11_t_all);    // https://github.com/olikraus/u8g2/wiki/fntlistall for fonts
  u8g2.drawStr(2, 22, "Starting engine");
  u8g2.sendBuffer();
  delay(2000);                            // give enough time to powerup peripherals

  Serial.println("Connecting to WiFi");
  u8g2.clearBuffer();
  u8g2.drawStr(2, 22, "Connecting WiFi");
  u8g2.sendBuffer();
  go_online();

  Serial.println("setting up eeprom");
  u8g2.clearBuffer();
  u8g2.drawStr(2, 22, "EEPROM setup");
  u8g2.sendBuffer();
  EEPROM.begin(EEPROM_SIZE);

  delay(2000); // Some delay to help some EEPROMs

  Serial.println("setting input pins");
  pinMode(rotaryDT, INPUT);
  pinMode(rotaryCLK, INPUT);

  ESP32Encoder::useInternalWeakPullResistors = UP;
  encoder.attachHalfQuad(rotaryDT, rotaryCLK);
  pinMode(rotarySW, INPUT_PULLUP);

  Serial.println("initialising decoder module");
  u8g2.clearBuffer();
  u8g2.drawStr(2, 22, "Decoder setup");
  u8g2.sendBuffer();
  stream.startDecoder(VS1053_CS, VS1053_DCS, VS1053_DREQ);

  Serial.println("reading last station from eeprom");
  radioStation = readStationFromEEPROM();
  if (radioStation > totalStations - 1) radioStation = 0; // if EEPROM has not been initialized
  Serial.println("last radio station:" + String(radioStation));
  encoder.setCount(radioStation);  // setting rotary encoder to station number

  Serial.println("setup done");
  u8g2.setFont(u8g2_font_profont17_mr);
}

void loop() {
  stream.loop();
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis > interval)
  {
    if (encoder.getCount() >= 0 && encoder.getCount() < totalStations) {
      radioStation = encoder.getCount();
    }
    else {
      if (encoder.getCount() < 0) {
        radioStation = totalStations - 1;
      }
      if (encoder.getCount() > totalStations - 1) {
        radioStation = 0;
      }
      encoder.setCount(radioStation);
    }

    if (radioStation != previousRadioStation)   // we are changing the station
    {
      currentState = 0;
      station_connect(radioStation);
      Serial.print("Number of station: ");
      Serial.println(radioStation);
      previousRadioStation = radioStation;
      seconds = 0;
    } else
    {
      seconds++;
      if (seconds == SECONDS_TO_AUTOSAVE)
      {
        int readStation = readStationFromEEPROM();
        if (readStation != radioStation)
        {
          writeStationToEEPROM();
        }
      }
    }
    previousMillis = currentMillis;

  }
  if ((!digitalRead(rotarySW))) {
    ESP.restart();   // button pressed, doing a restart
  }

  switch (currentState) {                          // non-blocking statemachine to allow button interrupt
    case 0:
      u8g2.clearBuffer();
      u8g2.setCursor(7, 22);
      u8g2.print(String(radioname[radioStation]));
      u8g2.drawLine(0, 0, 127, 0);
      u8g2.drawLine(0, 31, 127, 31);
      u8g2.sendBuffer();
      currentState++;
      break;
    case 1:
      if (wait(3000) == true)
      {
        currentState++;        // if lapsed, go to next step (state)
        shift = 128;           // start at the right end of the screen (for case 2)
      }
      break;
    case 2:
      u8g2.firstPage();
      do {
        u8g2.setCursor(shift--, 22);
        u8g2.print(songinfo);
        u8g2.drawLine(0, 0, 127, 0);
        u8g2.drawLine(0, 31, 127, 31);
      } while ( u8g2.nextPage() );
      if (shift == textwidth * -1) currentState = 0;
      break;
  }
}

void station_connect (int station_no ) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_profont17_mr);
  u8g2.setCursor(7, 22);
  u8g2.print ("buffering ...");
  u8g2.drawLine(0, 0, 127, 0);
  u8g2.drawLine(0, 31, 127, 31);
  u8g2.sendBuffer();
  stream.stopSong();
  Serial.println(String(radioname[radioStation]));
  stream.connecttohost(host[radioStation]);
}

void configModeCallback (WiFiManager * myWiFiManager) {
  u8g2.clearBuffer();
  u8g2.drawStr(12, 8,  "WiFi not configured");
  u8g2.drawStr(12, 20, "Connect to WLan:");
  u8g2.drawStr(12, 31, "--> WebRadio_AP");
  u8g2.sendBuffer();
}

void go_online() {

  WiFiManager wifiManager;
  wifiManager.setAPCallback(configModeCallback);

  if (!wifiManager.autoConnect("WebRadio_AP")) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    ESP.restart();   // no luck - going to restart the ESP
  }
  Serial.println("connected...yeey :)");

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Connected. Local IP: ");
    Serial.println(WiFi.localIP());
  }
  else {
    Serial.println("No WiFi connection!");
  }
}

int readStationFromEEPROM()
{
  int aa;
  aa = EEPROM.read(0);
  return aa;
}

void writeStationToEEPROM()
{
  Serial.println("Saving radio station to EEPROM: " + String(radioStation));
  EEPROM.write(0, radioStation);
  EEPROM.commit();
}
void audio_showstation(const char* info) {
  Serial.printf("showstation: %s\n", info);
}

void audio_showstreamtitle(const char* info) {
  Serial.printf("streamtitle: %s\n", info);
  songinfo = info;
  textwidth = u8g2.getUTF8Width(info);    // calculate the pixel width of the text
}

void audio_eof_stream(const char* info) {
  Serial.printf("eof: %s\n", info);
}
bool wait(unsigned long duration)
{
  static unsigned long startTime;
  static bool isStarted = false;

  if (isStarted == false)                  // if wait period not started yet
  {
    startTime = millis();                  // set the start time of the wait period
    isStarted = true;                      // indicate that it's started
    return false;                          // indicate to caller that wait period is in progress
  }
  if (millis() - startTime >= duration)    // check if wait period has lapsed
  {
    isStarted = false;    // lapsed, indicate no longer started so next time we call the function it will initialise the start time again
    return true;          // indicate to caller that wait period has lapsed
  }
  return false;           // indicate to caller that wait period is in progress
}

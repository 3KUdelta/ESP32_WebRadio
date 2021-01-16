//  Simple ESP32 web radio player.
//	Based on Nick Koumaris code - very nice job, Nick!
//  https://www.instructables.com/Internet-Radio-Using-an-ESP32/
//  https://github.com/educ8s/ESP32-Web-Radio-Simple

//  Customized by Marc Stähli / Jan 2021 / https://github.com/3KUdelta/ESP32_WebRadio
//  - added rotary knob to change channels
//  - added 128x32 oled display
//  - stereo version
//  - added treble and bass control
//  - added WiFiManger to config WiFi credentials over an access point
//  - added header file Stations.h for easier stations management

#include <VS1053.h>           //https://github.com/baldram/ESP_VS1053_Library
#include <ESP32Encoder.h>     //https://github.com/madhephaestus/ESP32Encoder
#include <HTTPClient.h>
#include <EEPROM.h>
#include <U8g2lib.h>          //https://github.com/olikraus/u8g2
#include <Wire.h>
#include <esp_wifi.h>
#include <HTTPClient.h>
#include <ESP_WiFiManager.h>  //https://github.com/khoih-prog/ESP_WiFiManager
#include "Stations.h"         // Radio station settings

#define VS1053_CS    32
#define VS1053_DCS   33
#define VS1053_DREQ  35

#define VOLUME       95         // volume level 0-100
#define EEPROM_SIZE  64

//   Bass/Treble: void setTone(uint8_t *rtone);
//
//   toneha       = <0..15>        // Setting treble gain (0 off, 1.5dB steps)
//   tonehf       = <0..15>        // Setting treble frequency lower limit x 1000 Hz
//   tonela       = <0..15>        // Setting bass gain (0 = off, 1dB steps)
//   tonelf       = <0..15>        // Setting bass frequency lower limit x 10 Hz
uint8_t rtone[4]  = {1, 14, 10, 15}; // initialize bass & treble

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

WiFiClient client;

uint8_t mp3buff[64];   // vs1053 likes 32 bytes at a time
VS1053 player(VS1053_CS, VS1053_DCS, VS1053_DREQ);

void(* resetFunc) (void) = 0;       // declare reset function @ address 0

void setup () {
  Serial.begin(115200); while (!Serial); delay(200);
  Serial.println("Starting up Web Radio");
  delay(500);
  Serial.println("initializing oled screen");
  delay(1000);

  SPI.begin();

  u8g2.begin();                           // initialize Screen
  u8g2.clearBuffer();                     // clear the internal memory
  u8g2.setFont(u8g2_font_t0_11_t_all);    // https://github.com/olikraus/u8g2/wiki/fntlistall for fonts
  u8g2.drawStr(2, 22, "Starting engine");
  u8g2.sendBuffer();
  delay(500);

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
  player.begin();
  player.switchToMp3Mode(); // make sure it is not in MIDI mode
  player.setVolume(VOLUME);
  player.setTone(rtone);

  Serial.println("reading last station from eeprom");
  radioStation = readStationFromEEPROM();
  if (radioStation > totalStations - 1) radioStation = 0; // if EEPROM has not been initialized
  Serial.println("last radio station:" + String(radioStation));
  encoder.setCount(radioStation);  // setting rotary encoder to station number

  Serial.println("setup done");
}

void loop() {

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

    if (radioStation != previousRadioStation)
    {
      station_connect(radioStation);
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
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_profont17_mr);
    u8g2.setCursor(7, 22);
    u8g2.print (String(radioname[radioStation]));
    u8g2.drawLine(0, 0, 127, 0);
    u8g2.drawLine(0, 31, 127, 31);
    u8g2.sendBuffer();

    if ((!digitalRead(rotarySW))) {
      resetFunc();        // doing a reset when rotary switch pressed
    }
  }

  if (client.available() > 0)
  {
    uint8_t bytesread = client.read(mp3buff, 64);
    player.playChunk(mp3buff, bytesread);
  }
}

void station_connect (int station_no ) {
  if (client.connect(host[station_no], port[station_no])) {
    Serial.print("Station connected : ");
    Serial.println(String(radioname[radioStation]));
  }
  client.print(String("GET ") + path[station_no] + " HTTP/1.1\r\n" +
               "Host: " + host[station_no] + "\r\n" +
               "Connection: close\r\n\r\n");
}

void go_online() {
  u8g2.clearBuffer();
  u8g2.drawStr(12, 8,  "WiFi not configured");
  u8g2.drawStr(12, 20, "Connect to WLan:");
  u8g2.drawStr(12, 31, "--> WebRadio_AP");
  u8g2.sendBuffer();
  ESP_WiFiManager ESP_wifiManager("WebRadio_AP");
  ESP_wifiManager.autoConnect("WebRadio_AP");
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Connected. Local IP: ");
    Serial.println(WiFi.localIP());
  }
  else {
    Serial.println(ESP_wifiManager.getStatus(WiFi.status()));
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

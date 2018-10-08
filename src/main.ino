#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <serialcmds.h>
#include <narrowbandcore.h>
#include <narrowband.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <Seeed_BME280.h>

// OLED Ports
#define BUTTON_A 4
#define BUTTON_B 3
#define BUTTON_C 8
#define LED 13

#define SEALEVELPRESSURE_HPA (1013.25)
#define LED1 A1

Adafruit_SSD1306 display = Adafruit_SSD1306();
HardwareSerial& modem_serial = Serial1;
BME280 bme280;
// Instantiate command adapter as the connection via serial
Narrowband::ArduinoSerialCommandAdapter ca(modem_serial);

// Driver class
Narrowband::NarrowbandCore nbc(ca);

Narrowband::FunctionConfig cfg;

// instantiate NB object.
Narrowband::Narrowband nb(nbc, cfg);

void setup() {
    // connection speed to your terminal (e.g. via USB)
    Serial.begin(115200);
    Serial.println("Hit [ENTER] or wait 10s ");

    Serial.setTimeout(10000);
    Serial.readStringUntil('\n');
    if(!bme280.init()) {
        Serial.println("Error initializing BME280 Sensor.");
      } else {
          Serial.println("BME280 is initialized.");
      }

      //OLED init
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    Serial.println("OLED is initalized.");
    display.display();
    delay(1000);

    pinMode(LED1, OUTPUT);
    pinMode(BUTTON_A, INPUT_PULLUP);
    pinMode(BUTTON_B, INPUT_PULLUP);
    pinMode(BUTTON_C, INPUT_PULLUP);

    // Begin modem serial communication with correct speed (check shield specs!)
    // TEKMODUL BC68-DEVKIT 9600,echo
    modem_serial.begin(9600);
    delay(3000);
}

void loop() {

  // Quectel: wait at least 10 seconds
  delay(10*1000);

  int all_bands[10] = {0};
  size_t num_all_bands = 0;

  // query all bands module supports
  nb.getCore().getSupportedBands(all_bands, 10, num_all_bands);

  Serial.print("Supported Bands: ");
  for ( size_t i = 0; i < num_all_bands; i++) {
      Serial.print(all_bands[i]); Serial.print(" ");
  }
  Serial.println();
  delay(1000);

  // begin session
  nb.begin();
  if (!nb) {
      Serial.println("Error initializing NB module.");
      return;
  } else {
      Serial.println("NB module Initialized.");
  }
  nb.getCore().setEcho(false);
/*
  int bands2[1] = { 1 };
  nb.getCore().setBands(bands2, sizeof(bands2));
*/
int cp_arr[] = { 0 };

  for ( size_t i = 0; i < num_all_bands; i++ ){
      cp_arr[0] = all_bands[i];

      nb.getCore().setModuleFunctionality(false);

      if ( nb.getCore().setBands(cp_arr, 1)) {
        Serial.println("Successfully set band");
      } else {
        Serial.println("Error setting band");
      }

      nb.getCore().setModuleFunctionality(true);

      // query currently activated band
      int bands[10];
      size_t num_bands;
      nb.getCore().getBands(bands, 10, num_bands);

      Serial.print("Currently active Band: ");
      for ( size_t i = 0; i < num_bands; i++) {
          Serial.print(bands[i]); Serial.print(" ");
      }
      Serial.println();
      delay(1000);

      Serial.print("Trying to attach to B");
      Serial.println(cp_arr[0]);

      if (nb.attach(15000)) {
          Serial.println("Attachment Successful!");

          String ip;
          if (nb.getCore().getPDPAddress(ip)) {
              Serial.println(ip);
          }
          delay(2000);

          // request something. Put in your IP address and
          // request data.
          String req("hello world"), resp;
          /*if ( nb.sendReceiveUDP("192.168.0.1", 9876,req,resp)) {
              Serial.println(resp);
          } */

          // Write to OLED
          display.clearDisplay();
          display.display();

          // text display tests
          display.setTextSize(1);
          display.setTextColor(WHITE);
          display.setCursor(0,0);
          display.println("ThingForward.io");
          display.print("IP: ");
          display.println(ip);
          display.print("Active Band: B");
          display.println(cp_arr[0]);
          display.print("Today is: ");
          display.print(bme280.getTemperature());
          display.println(" C");

          display.setCursor(0,0);
          display.display();
          // End of Writing to OLED


          String msg("89754C84BC46918C?Wtemp=");
          // get temperature from bme280 and send it via UDP
          msg += bme280.getTemperature();
          nb.sendUDP("40.114.225.189", 9876, msg);
          delay(2000);
          String msg2("89754C84BC46918C?Whumd=");
          // get humidity from bme280 and send it via UDP
          msg2 += bme280.getHumidity();
          nb.sendUDP("40.114.225.189", 9876, msg2);
          delay(2000);
          String msg3("89754C84BC46918C?Wpres=");
          // get pressure from bme280 and send it via UDP
          msg3 += bme280.getPressure();
          nb.sendUDP("40.114.225.189", 9876, msg3);
          delay(2000);

          nb.detach();

      } else {
          Serial.println("Unable to attach to NB Network!");
      }
//      Serial.println("NB Network Attachment Error.");
  }

      nb.end();
      //delay(1000*60*14);
      Serial.println("Session is ended.");
      //Every 15 minutes the data will be sent
}

/*
  FILE: ATdebug.ino
  AUTHOR: Kaibin
  PURPOSE: Test functionality
*/

#define TINY_GSM_MODEM_SIM7600
#define TINY_GSM_RX_BUFFER 1024 // Set RX buffer to 1Kb
#define SerialAT Serial1

// See all AT commands, if wanted
#define DUMP_AT_COMMANDS

// set GSM PIN, if any
#define GSM_PIN ""

// Your GPRS credentials, if any
const char apn[] = "YOUR-APN"; // SET TO YOUR APN
const char gprsUser[] = "";
const char gprsPass[] = "";

#include <TinyGsmClient.h>
#include <SPI.h>
#include <SD.h>
#include <Ticker.h>
#include "Freenove_WS2812_Lib_for_ESP32.h"
#include "utilities.h"

#ifdef DUMP_AT_COMMANDS // if enabled it requires the streamDebugger lib
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, Serial);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif

Freenove_ESP32_WS2812 strip = Freenove_ESP32_WS2812(1, LED_PIN, 0, TYPE_GRB);

int counter, lastIndex, numberOfPieces = 24;
String pieces[24], input;

bool reply = false;

void modem_on()
{

  /*
  MODEM_PWRKEY IO:4 The power-on signal of the modulator must be given to it,
  otherwise the modulator will not reply when the command is sent
  */
  pinMode(MODEM_PWRKEY, OUTPUT);
  digitalWrite(MODEM_PWRKEY, HIGH);
  delay(300); // Need delay
  digitalWrite(MODEM_PWRKEY, LOW);

  int i = 10;
  Serial.println("\nTesting Modem Response...\n");
  Serial.println("****");
  while (i)
  {
    SerialAT.println("AT");
    delay(500);
    if (SerialAT.available())
    {
      String r = SerialAT.readString();
      Serial.println(r);
      if (r.indexOf("OK") >= 0)
      {
        reply = true;
        break;
        ;
      }
    }
    delay(500);
    i--;
  }
  Serial.println("****\n");
}

void setup()
{
  Serial.begin(115200); // Set console baud rate
  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(100);
  strip.begin();
  strip.setBrightness(10);
  strip.setLedColorData(0, 0xe5, 0xe5, 0xe5);

  strip.show();

  modem_on();
  if (reply)
  {
    Serial.println(F("***********************************************************"));
    Serial.println(F(" You can now send AT commands"));
    Serial.println(F(" Enter \"AT\" (without quotes), and you should see \"OK\""));
    Serial.println(F(" If it doesn't work, select \"Both NL & CR\" in Serial Monitor"));
    Serial.println(F(" DISCLAIMER: Entering AT commands without knowing what they do"));
    Serial.println(F(" can have undesired consiquinces..."));
    Serial.println(F("***********************************************************\n"));
  }
  else
  {
    Serial.println(F("***********************************************************"));
    Serial.println(F(" Failed to connect to the modem! Check the baud and try again."));
    Serial.println(F("***********************************************************\n"));
  }
}

void loop()
{
  while (true)
  {
    if (SerialAT.available())
    {
      Serial.write(SerialAT.read());
    }
    if (Serial.available())
    {
      SerialAT.write(Serial.read());
    }
    delay(1);
  }
}

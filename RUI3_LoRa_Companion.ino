/*
  RUI3 LoRa Companion
  A small LoRa Dongle for RUI3 – works on RAK4631-R and RAK3172.
  Docs center: https://docs.rakwireless.com/RUI3/
  Installing RUI3: https://docs.rakwireless.com/RUI3/Supported-IDE/
*/

#undef max
#undef min
#include <string>
#include <vector>
#include <cstring>

using namespace std;

void recv_cb(rui_lora_p2p_recv_t);
void send_cb();
void displayScroll(char *);
void hexDump(uint8_t*, uint16_t);

#include <ArduinoJson.h> // Click here to get the library: http://librarymanager/All#ArduinoJson By Benoit Blanchon
#include "OLED_helper.h" // See this tab for libraries to install
#include "helper.h"
#include "nRF_AES.h"
#include "nRF_Random.h"
#include "nRF_Hash.h"
#include "Settings.h"
#include "lora_helper.h"
#include "Commands.h"

double lastCheck;

void setup() {
  delay(1000);
  Serial.begin(115200, RAK_CUSTOM_MODE);
  // RAK_CUSTOM_MODE disables AT firmware parsing
  delay(5000);
  uint8_t x = 5;
  while (x > 0) {
    Serial.printf("%d, ", x--);
    delay(500);
  } // Just for show
  Serial.println("0!");
  /*
    5, 4, 3, 2, 1, 0!
    LoRa Companion
    RUI3 version: RUI_3.5.0b_165_RAK4631
    LoRa Setup
    P2P mode: ok
    Freq 868.125: ok
    SF 12: ok
    BW 0, 125 KHz: x
    Lib begin
    AES begin
    RND begin
    My Name: C0F15EAE0947
  */
  Serial.println("LoRa Companion");
  strcpy(version, api.system.firmwareVersion.get().c_str());
  Serial.printf("RUI3 version: %s\n", version);
  if (version[6] < '5') {
    fullBW = false;
    maxBW = 2;
  }
  Serial.printf("Full BW version: %s\n", fullBW ? "yes" : "no");

  int rc = oledInit(&ssoled, MY_OLED, OLED_ADDR, FLIP180, INVERT, USE_HW_I2C, SDA_PIN, SCL_PIN, RESET_PIN, 400000L); // use standard I2C bus at 400Khz
  if (rc != OLED_NOT_FOUND) {
    oledFill(&ssoled, 0, 1);
    oledWriteString(&ssoled, 0, 1, 0, "LoRa Companion", FONT_NORMAL, 0, 1);
    oledSetBackBuffer(&ssoled, ucBackBuffer);
    delay(2000);
    hasOLED = true;
  }
  Serial.println("LoRa Setup");
  delay(1000);
  bool rslt = setupLoRa();
  if (!rslt && hasOLED) {
    oledFill(&ssoled, 1, 1);
    oledWriteString(&ssoled, 0, 1, 0, "LoRa Companion", FONT_NORMAL, 1, 1);
    oledWriteString(&ssoled, 0, 5, 2, "Ouch!", FONT_LARGE, 1, 1);
    oledWriteString(&ssoled, 0, 1, 4, "Setup error", FONT_NORMAL, 1, 1);
    oledWriteString(&ssoled, 0, 0, 5, "Stopping here...", FONT_NORMAL, 1, 1);
    oledDumpBuffer(&ssoled, NULL);
    while (true);
  }
  setupAES();
  if (hasOLED) {
    oledFill(&ssoled, 0, 1);
    for (uint8_t ix = 0; ix < 10; ix++) {
      oledWriteString(&ssoled, 0, ix, 0, " READY ", FONT_LARGE, 0, 1);
      delay(100);
    }
    posY = 3;
  }
  sprintf(myName, "%s", api.ble.mac.get());
  Serial.printf("My Name: %s\n", myName);
  lastCheck = millis();
  cmdCount = sizeof(cmds) / sizeof(myCommand);
} /* setup() */

void loop() {
  if (millis() - lastCheck > 10000) {
    // do something
    lastCheck = millis();
  }
  if (Serial.available()) {
    // incoming from user
    char incoming[256];
    memset(incoming, 0, 256);
    uint8_t ix = 0;
    while (Serial.available()) {
      char c = Serial.read();
      delay(25);
      if (c == 13 || c == 10) {
        // cr / lf
        if (ix > 0) {
          incoming[ix] = 0;
          string nextLine = string(incoming);
          userStrings.push_back(nextLine);
          ix = 0;
        }
      } else incoming[ix++] = c;
    }
  }
  if (userStrings.size() > 0) {
    uint8_t ix, iy = userStrings.size();
    for (ix = 0; ix < iy; ix++) {
      if (userMode == 0) sendString(userStrings[ix]);
      else if (userMode == 1) evalString(userStrings[ix]);
    }
    userStrings.resize(0);
  }
}

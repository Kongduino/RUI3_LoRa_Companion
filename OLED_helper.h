#include <ss_oled.h>

extern SSOLED ssoled;
bool hasOLED = false;

int posY = 2; // First 2 lines are reserved for the title.
// Since displayScroll starts with incrementing posY, we start with posY = 1
void displayScroll(char *msgToSend) {
  if(!hasOLED) return;
  posY += 1;
  if (posY == 8) {
    posY = 7; // keep it at 7, the last line
    for (uint8_t i = 0; i < 8; i++) {
      // and scroll, one pixel line – not text line – at a time.
      oledScrollBuffer(&ssoled, 0, 127, 3, 7, 1);
      oledDumpBuffer(&ssoled, NULL);
    }
  }
  // then display text
  oledWriteString(&ssoled, 0, 0, posY, msgToSend, FONT_8x8, 0, 1);
}

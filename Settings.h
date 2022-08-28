double myFreq = 868125000;
uint16_t sf = 12;
uint16_t bw = 0;
uint16_t cr = 0;
uint16_t preamble = 8;
uint16_t txPower = 22;
bool needAES = false;
bool needJSON = false;
uint8_t myPWD[16];
uint8_t myIV[16];
char myName[32] = {0};

// ------------------------------------------------------------------
// Do not edit below
// ------------------------------------------------------------------
// I mean it!
// ------------------------------------------------------------------
// Pre-3.5.0b:
uint8_t maxBW = 9;
// There's a bug in the API – BW values below 125 haven't been implemented.
// 3.5.0b and onwards:
uint32_t myBWs[10] = {125, 250, 500, 7.8, 10.4, 15.63, 20.83, 31.25, 41.67, 62.5};

static uint8_t ucBackBuffer[1024];
#define SDA_PIN -1
#define SCL_PIN -1
#define RESET_PIN -1
#define OLED_ADDR 0x3c
#define FLIP180 0
#define INVERT 0
#define USE_HW_I2C true
#define MY_OLED OLED_128x64
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
SSOLED ssoled;
char msg[128]; // general-use buffer
nRFCrypto_AES aes;
nRFCrypto_Random rnd;
nRFCrypto_Hash myHash;

// ------------------------------------------------------------------
void setupAES() {
  SaSi_LibInit();
  sprintf(msg, " * Lib begin");
  Serial.println(msg);
  displayScroll(msg);

  aes.begin();
  sprintf(msg, " * AES begin");
  Serial.println(msg);
  displayScroll(msg);

  rnd.begin();
  sprintf(msg, " * RND begin");
  Serial.println(msg);
  displayScroll(msg);
}

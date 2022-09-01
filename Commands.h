#include <stdio.h>
#include <string.h>

void handleHelp(char *);
void handleFreq(char *);
void handleBW(char *);
void handleSF(char *);
void handleCR(char *);
void handleTX(char *);
void handleP2P(char *);
void handleAES(char*);
void handlePassword(char*);
void handleIV(char*);
void sendBlock(uint8_t*, uint8_t);
void handleJSON(char*);

vector<string> userStrings;
uint8_t userMode = 0;
bool isSending = false;

int cmdCount = 0;
struct myCommand {
  void (*ptr)(char *); // Function pointer
  char name[12];
  char help[48];
};

myCommand cmds[] = {
  {handleHelp, "help", "Shows this help."},
  {handleP2P, "p2p", "Shows the P2P settings."},
  {handleFreq, "fq", "Gets/sets the working frequency."},
  {handleBW, "bw", "Gets/sets the working bandwidth."},
  {handleSF, "sf", "Gets/sets the working spreading factor."},
  {handleCR, "cr", "Gets/sets the working coding rate."},
  {handleTX, "tx", "Gets/sets the working TX power."},
  {handleAES, "aes", "Gets/sets AES encryption status."},
  {handlePassword, "pwd", "Gets/sets AES password."},
  {handleIV, "iv", "Gets/sets AES IV."},
  {handleJSON, "json", "Gets/sets JSON sending status."},
};

void handleHelp(char *param) {
  Serial.printf("Available commands: %d\n", cmdCount);
  for (int i = 0; i < cmdCount; i++) {
    sprintf(msg, " . %s: %s", cmds[i].name, cmds[i].help);
    Serial.println(msg);
  }
}

void evalCmd(char *str, string fullString) {
  uint8_t ix, iy = strlen(str);
  for (ix = 0; ix < iy; ix++) {
    char c = str[ix];
    // lowercase the keyword
    if (c >= 'A' && c <= 'Z') str[ix] = c + 32;
  }
  Serial.print("Evaluating: `");
  Serial.print(fullString.c_str());
  Serial.println("`");
  for (int i = 0; i < cmdCount; i++) {
    if (strcmp(str, cmds[i].name) == 0) {
      cmds[i].ptr((char*)fullString.c_str());
      return;
    }
  }
}

void handleCommands(string str1) {
  char kwd[32];
  int i = sscanf((char*)str1.c_str(), "%s", kwd);
  if (i > 0) evalCmd(kwd, str1);
  else handleHelp("");
}

void evalString(string userString) {
  if (userString.compare("$$$") == 0) {
    Serial.println("Switching back to transparent mode!");
    userMode = 0;
    return;
  }
  handleCommands(userString);
}

void sendString(string userString) {
  if (userString.compare("$$$") == 0) {
    Serial.println("Switching to command mode!");
    userMode = 1;
    return;
  }
  /*
    TODO: check whether needJSON is true,
    And if so, transform userString into a JSON packet:
    {"UUID":"xxxxxxxx","from":"<something>","cmd":"<userString>"}
    from could be the chip's UUID
  */
  if (needJSON) {
    uint8_t tempUUID[4];
    char plainUUID[9] = {0};
    rnd.generate(tempUUID, 4);
    array2hex(tempUUID, 4, plainUUID);
    plainUUID[8] = 0;

    DynamicJsonDocument obj(512);
    obj["UUID"] = plainUUID;
    obj["from"] = myName;
    obj["cmd"] = userString.c_str();
    String myString;
    serializeJson(obj, myString);
    userString = myString.c_str();
  }
  uint8_t myLen = aes.blockLen(userString.size());
  char plainText[myLen + 1] = {0};
  strcpy(plainText, (char*)userString.c_str());
  if (!needAES) sendMsg(plainText);
  else {
    // A function that calculates the required length. √
    char encBuf[myLen + 1] = {0}; // Let's make sure we have enough space for the encrypted string
    int rslt = aes.Process(plainText, myLen, myIV, myPWD, 16, encBuf, aes.encryptFlag, aes.ecbMode);
    if (rslt < 0) {
      Serial.printf("Error %d in Process ECB Encrypt\n", rslt);
      return;
    }
    // Serial.println("ECB Encrypted:");
    // hexDump((uint8_t*)encBuf, myLen);
    sendBlock((uint8_t*)encBuf, myLen);
  }
}

void handleFreq(char *param) {
  if (strcmp("fq", param) == 0) {
    // no parameters
    sprintf(msg, "Frequency: %.3f MHz\n", (myFreq / 1e6));
    Serial.print(msg);
    sprintf(msg, "Fq: %.3f MHz\n", (myFreq / 1e6));
    displayScroll(msg);
    return;
  } else {
    // fq xxx.xxx set frequency
    float value = atof(param + 2);
    // for some reason sscanf returns 0.000 as value...
    if (value < 150.0 || value > 960.0) {
      // sx1262 freq range 150MHz to 960MHz
      // Your chip might not support all...
      sprintf(msg, "Invalid frequency value: %.3f\n", value);
      Serial.print(msg);
      return;
    }
    myFreq = value * 1e6;
    api.lorawan.precv(0);
    // turn off reception
    sprintf(msg, "Set P2P frequency to %3.3f: %s MHz\n", (myFreq / 1e6), api.lorawan.pfreq.set(myFreq) ? "Success" : "Fail");
    Serial.print(msg);
    api.lorawan.precv(65533);
    sprintf(msg, "New freq: %.3f", value);
    displayScroll(msg);
    return;
  }
}

void handleBW(char*param) {
  int value;
  int i = sscanf(param, "%*s %d", &value);
  if (strcmp("bw", param) == 0) {
    // no parameters
    if (fullBW)
      sprintf(msg, "bandwidth [f]: %d, ie %d KHz vs %d, ie %d KHz\n", bw, myBWs[bw], api.lorawan.pbw.get(), myBWs[api.lorawan.pbw.get()]);
    else
      sprintf(msg, "bandwidth [3]: %d, ie %d KHz vs %d KHz\n", bw, myBWs[bw], api.lorawan.pbw.get());
    Serial.print(msg);
    sprintf(msg, "BW: %d KHz", myBWs[bw]);
    displayScroll(msg);
    return;
  } else {
    // bw xxxx set BW
    value = atof(param + 2);
    if (value > maxBW) {
      sprintf(msg, "Invalid BW value: %d [max: %d]\n", value, maxBW);
      Serial.print(msg);
      return;
    }
    bw = value;
    if (fullBW) sprintf(msg, "Set bandwidth to %d/%d: %s\n", bw, myBWs[bw], api.lorawan.pbw.set(bw) ? "Success" : "Fail");
    else sprintf(msg, "Set bandwidth to %d/%d: %s\n", bw, myBWs[bw], api.lorawan.pbw.set(myBWs[bw]) ? "Success" : "Fail");
    api.lorawan.precv(0);
    // turn off reception
    
    Serial.print(msg);
    api.lorawan.precv(65533);
    sprintf(msg, "New BW: %d", myBWs[bw]);
    displayScroll(msg);
    return;
  }
}

void handleSF(char*param) {
  int value;
  int i = sscanf(param, "%*s %d", &value);
  if (i == -1) {
    // no parameters
    sprintf(msg, "SF: %d\n", sf);
    Serial.print(msg);
    sprintf(msg, "SF: %d", sf);
    displayScroll(msg);
    return;
  } else {
    // sf xxxx set SF
    if (value < 5 || value > 12) {
      sprintf(msg, "Invalid SF value: %d\n", value);
      Serial.print(msg);
      return;
    }
    sf = value;
    api.lorawan.precv(0);
    // turn off reception
    sprintf(msg, "Set P2P spreading factor to %d: %s\n", sf, api.lorawan.psf.set(sf) ? "Success" : "Fail");
    Serial.print(msg);
    api.lorawan.precv(65533);
    sprintf(msg, "SF set to %d", sf);
    displayScroll(msg);
    return;
  }
}

void handleCR(char*param) {
  int value;
  int i = sscanf(param, "%*s %d", &value);
  if (i == -1) {
    // no parameters
    sprintf(msg, "CR: 4/%d\n", (cr + 5));
    Serial.print(msg);
    sprintf(msg, "CR: 4/%d", (cr + 5));
    displayScroll(msg);
    return;
  } else {
    // sf xxxx set SF
    if (value < 5 || value > 8) {
      sprintf(msg, "Invalid CR value: %d\n", value);
      Serial.print(msg);
      return;
    }
    cr = value - 5;
    api.lorawan.precv(0);
    // turn off reception
    sprintf(msg, "Set P2P coding rate to %d: %s\n", cr, api.lorawan.pcr.set(cr) ? "Success" : "Fail");
    Serial.print(msg);
    api.lorawan.precv(65533);
    sprintf(msg, "CR set to 4/%d", (cr + 5));
    displayScroll(msg);
    return;
  }
}

void handleTX(char*param) {
  int value;
  int i = sscanf(param, "%*s %d", &value);
  if (i == -1) {
    // no parameters
    sprintf(msg, "TX power: %d\n", txPower);
    Serial.print(msg);
    sprintf(msg, "Tx pwr: %d", txPower);
    displayScroll(msg);
    return;
  } else {
    // sf xxxx set SF
    if (value < 5 || value > 22) {
      sprintf(msg, "Invalid TX power value: %d\n", value);
      Serial.print(msg);
      return;
    }
    txPower = value;
    api.lorawan.precv(0);
    // turn off reception
    sprintf(msg, "Set P2P Tx power to %d: %s\n", cr, api.lorawan.ptp.set(txPower) ? "Success" : "Fail");
    Serial.print(msg);
    api.lorawan.precv(65533);
    sprintf(msg, "Tx pwr set to %d", txPower);
    displayScroll(msg);
    return;
  }
}

void handleAES(char* param) {
  int value;
  int i = sscanf(param, "%*s %d", &value);
  if (i == -1) {
    // no parameters
    sprintf(msg, "AES: %s\n", needAES ? "on" : "off");
    Serial.print(msg);
    displayScroll(msg);
    return;
  } else {
    // aes 0 = off, aes 1 = on
    if (value < 0 || value > 1) {
      sprintf(msg, "Invalid value: %d. Use 0 for OFF or 1 for ON.\n", value);
      Serial.print(msg);
      return;
    }
    needAES = value == 1;
    sprintf(msg, "AES: %s", needAES ? "on" : "off");
    displayScroll(msg);
    Serial.println(msg);
    return;
  }
}

void handleP2P(char *param) {
  float f0 = myFreq / 1e6, f1 = api.lorawan.pfreq.get() / 1e6;
  // check stored value vs real value
  Serial.println("+---------------+-----------+-----------+");
  sprintf(msg, "|%-15s|%-11s|%-11s|\n", "     Value", " in-memory", "    chip");
  Serial.print(msg);
  Serial.println("+---------------+-----------+-----------+");
  sprintf(msg, "Fq: %.3f MHz\n", f1);
  displayScroll(msg);
  sprintf(msg, "|%-15s|%-11d|%-11d|\n", "SF", sf, api.lorawan.psf.get());
  Serial.print(msg);
  displayScroll(msg);
  if (fullBW) sprintf(msg, "|%-15s|%d: %d KHz |%d: %d KHz |\n", "Bandwidth", bw, myBWs[bw], api.lorawan.pbw.get(), myBWs[api.lorawan.pbw.get()]);
  else sprintf(msg, "|%-15s|%2d: %d KHz|%2d: %d KHz|\n", bw, myBWs[bw], api.lorawan.pbw.get());
  Serial.print(msg);
  sprintf(msg, "BW: %d KHz", bw);
  displayScroll(msg);
  sprintf(msg, "|%-15s|%-11d|%-11d|\n", "CR 4/", (cr + 5), (api.lorawan.pcr.get() + 5));
  Serial.print(msg);
  displayScroll(msg);
  sprintf(msg, "|%-15s|%-11d|%-11d|\n", "Tx Power", txPower, api.lorawan.ptp.get());
  Serial.print(msg);
  sprintf(msg, "TX power: %d", txPower);
  displayScroll(msg);
  Serial.println("+---------------+-----------+-----------+");
}

void handlePassword(char* param) {
  char pwd[33];
  memset(pwd, 0, 33);
  int i = sscanf(param, "%*s %s", pwd);
  if (i == -1) {
    // no parameters
    sprintf(msg, "pwd: yeah right");
    Serial.println(msg);
    displayScroll(msg);
    return;
  } else {
    // either 16 chars or 32 hex chars
    if (strlen(pwd) == 16) {
      memcpy(myPWD, pwd, 16);
      sprintf(msg, "Password set.");
      Serial.println(msg);
      displayScroll(msg);
      // hexDump(myPWD, 16);
      return;
    } else if (strlen(pwd) == 32) {
      hex2array(pwd, myPWD, 32);
      sprintf(msg, "Password set.");
      Serial.println(msg);
      displayScroll(msg);
      // hexDump(myPWD, 16);
      return;
    }
    sprintf(msg, "AES: wrong pwd size!");
    displayScroll(msg);
    Serial.println(msg);
    sprintf(msg, "(Should be 16 bytes)");
    displayScroll(msg);
    Serial.println(msg);
    return;
  }
}

void handleIV(char* param) {
  char iv[33];
  memset(iv, 0, 33);
  int i = sscanf(param, "%*s %s", iv);
  if (i == -1) {
    // no parameters
    sprintf(msg, "IV: yeah right");
    Serial.println(msg);
    displayScroll(msg);
    return;
  } else {
    // either 16 chars or 32 hex chars
    if (strlen(iv) == 16) {
      memcpy(myIV, iv, 16);
      sprintf(msg, "IV set.");
      Serial.println(msg);
      displayScroll(msg);
      // hexDump(myIV, 16);
      return;
    } else if (strlen(iv) == 32) {
      hex2array(iv, myIV, 32);
      sprintf(msg, "IV set.");
      Serial.println(msg);
      displayScroll(msg);
      // hexDump(myIV, 16);
      return;
    }
    sprintf(msg, "AES: wrong IV size!");
    displayScroll(msg);
    Serial.println(msg);
    sprintf(msg, "(Should be 16 bytes)");
    displayScroll(msg);
    Serial.println(msg);
    return;
  }
}

void handleJSON(char* param) {
  int value;
  int i = sscanf(param, "%*s %d", &value);
  if (i == -1) {
    // no parameters
    sprintf(msg, "JSON: %s\n", needJSON ? "on" : "off");
    Serial.print(msg);
    displayScroll(msg);
    return;
  } else {
    // json 0 = off, json 1 = on
    if (value < 0 || value > 1) {
      sprintf(msg, "Invalid value: %d. Use 0 for OFF or 1 for ON.\n", value);
      Serial.print(msg);
      return;
    }
    needJSON = value == 1;
    sprintf(msg, "JSON: %s", needJSON ? "on" : "off");
    displayScroll(msg);
    Serial.println(msg);
    return;
  }
}

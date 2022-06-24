#include <stdio.h>
#include <string.h>

void handleHelp(char *);
void handleFreq(char *);
void handleBW(char *);
void handleSF(char *);
void handleCR(char *);
void handleTX(char *);
void handleP2P(char *);

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
  sendMsg((char*)userString.c_str());
}

void handleFreq(char *param) {
  if (strcmp("fq", param) == 0) {
    // no parameters
    sprintf(msg, "P2P frequency: %.3f MHz\n", (myFreq / 1e6));
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
    sprintf(msg, "P2P bandwidth: %d KHz\n", bw);
    Serial.print(msg);
    sprintf(msg, "BW: %d KHz", bw);
    displayScroll(msg);
    return;
  } else {
    // bw xxxx set BW
    value = atof(param + 2);
    if (value > 9) {
      sprintf(msg, "Invalid BW value: %d\n", value);
      Serial.print(msg);
      return;
    }
    bw = myBWs[value];
    api.lorawan.precv(0);
    // turn off reception
    sprintf(msg, "Set P2P bandwidth to %d/%d: %s\n", value, bw, api.lorawan.pbw.set(bw) ? "Success" : "Fail");
    Serial.print(msg);
    api.lorawan.precv(65533);
    sprintf(msg, "New BW: %d", bw);
    displayScroll(msg);
    return;
  }
}

void handleSF(char*param) {
  int value;
  int i = sscanf(param, "%*s %d", &value);
  if (i == -1) {
    // no parameters
    sprintf(msg, "P2P SF: %d\n", sf);
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
  int i = sscanf(param, "/%*s %d", &value);
  if (i == -1) {
    // no parameters
    sprintf(msg, "P2P CR: 4/%d\n", (cr + 5));
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
  int i = sscanf(param, "/%*s %d", &value);
  if (i == -1) {
    // no parameters
    sprintf(msg, "P2P TX power: %d\n", txPower);
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

void handleP2P(char *param) {
  float f0 = myFreq / 1e6, f1 = api.lorawan.pfreq.get() / 1e6;
  // check stored value vs real value
  sprintf(msg, "P2P frequency: %.3f/%.3f MHz\n", f0, f1);
  Serial.print(msg);
  sprintf(msg, "Fq: %.3f MHz\n", f1);
  displayScroll(msg);
  sprintf(msg, "P2P SF: %d\n", sf);
  Serial.print(msg);
  displayScroll(msg);
  sprintf(msg, "P2P bandwidth: %d KHz\n", bw);
  Serial.print(msg);
  sprintf(msg, "BW: %d KHz", bw);
  displayScroll(msg);
  sprintf(msg, "P2P C/R: 4/%d\n", (cr + 5));
  Serial.print(msg);
  displayScroll(msg);
  sprintf(msg, "P2P TX power: %d\n", txPower);
  Serial.print(msg);
  sprintf(msg, "TX power: %d", txPower);
  displayScroll(msg);
}

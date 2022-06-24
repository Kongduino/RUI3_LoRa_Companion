extern SSOLED ssoled;
extern bool isSending;
int posY = 2; // First 2 lines are reserved for the title.
// Since displayScroll starts with incrementing posY, we start with posY = 1

void send_cb(void) {
  // TX callback
  Serial.println("Tx done!");
  // Serial.printf("reset Rx mode[65533] %s\n", api.lorawan.precv(65533) ? "ok" : "x");
  // set the LoRa module to indefinite listening mode:
  // no timeout + no limit to the number of packets
  // NB: 65535 = wait for ONE packet, no timeout
  // - If api.lorawan.precv(65534) is called, the device will be set as RX mode until api.lorawan.precv(0) is called.
  // If api.lorawan.precv(65533) is called, the device will be set as RX mode, but still can do TX without calling api.lorawan.precv(0).
  isSending = false;
}

void recv_cb(rui_lora_p2p_recv_t data) {
  uint16_t ln = data.BufferSize;
  hexDump(data.Buffer, ln);
  char buff[92];
  sprintf(buff, "Incoming message, length: %d, RSSI: %d, SNR: %d", data.BufferSize, data.Rssi, data.Snr);
  Serial.println(buff);
  sprintf(buff, "%d-byte packet", data.BufferSize);
  oledFill(&ssoled, 1, 1);
  oledDumpBuffer(&ssoled, NULL);
  oledWriteString(&ssoled, 0, 3, 0, "Incoming!", FONT_NORMAL, 0, 1);
  oledWriteString(&ssoled, 0, 0, 1, buff, FONT_NORMAL, 0, 1);
  sprintf(buff, "RSSI: %d", data.Rssi);
  oledWriteString(&ssoled, 0, 0, 2, buff, FONT_NORMAL, 0, 1);
  sprintf(buff, "SNR: %d", data.Snr);
  oledWriteString(&ssoled, 0, 0, 3, buff, FONT_NORMAL, 0, 1);

  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, data.Buffer);
  if (!error) {
    const char *myID = doc["UUID"];
    Serial.print(" * ID: ");
    Serial.println(myID);
    sprintf(buff, "ID: %s", (char*)myID);
    oledWriteString(&ssoled, 0, 0, 4, buff, FONT_NORMAL, 0, 1);

    // Print sender
    const char *from = doc["from"];
    Serial.print(" * Sender: ");
    Serial.println(from);
    sprintf(buff, "from: %s", (char*)from);
    oledWriteString(&ssoled, 0, 0, 5, buff, FONT_NORMAL, 0, 1);

    // Print command
    const char *cmd = doc["cmd"];
    Serial.print(" * Command: ");
    Serial.println(cmd);
    sprintf(buff, "cmd: %s", (char*)cmd);
    oledWriteString(&ssoled, 0, 0, 6, buff, FONT_NORMAL, 0, 1);
    posY = 6;
    return; // End for JSON messages
  }
  // Let's pretty print the message.
  oledSetTextWrap(&ssoled, true);
  oledWriteString(&ssoled, 0, 0, 4, (char*)data.Buffer, FONT_NORMAL, 0, 1);
  Serial.println("Message:");
  char msg[data.BufferSize + 1];
  memset(msg, 0, data.BufferSize + 1);
  memcpy(msg, data.Buffer, data.BufferSize);
  Serial.println(msg);
  posY = 6;
}

bool setupLoRa() {
  // LoRa setup – everything else has been done for you. No need to fiddle with pins, etc
  bool rslt = api.lorawan.nwm.set(0);
  sprintf(msg, "P2P mode: %s", rslt ? "ok" : "x");
  Serial.println(msg);
  displayScroll(msg);
  if (!rslt) return false;

  rslt = api.lorawan.pfreq.set(myFreq);
  sprintf(msg, "Freq %3.3f: %s", (myFreq / 1e6), rslt ? "ok" : "x");
  Serial.println(msg);
  displayScroll(msg);
  if (!rslt) return false;

  rslt = api.lorawan.psf.set(sf);
  sprintf(msg, "SF %d: %s", sf, rslt ? "ok" : "x");
  Serial.println(msg);
  displayScroll(msg);
  if (!rslt) return false;

  rslt = api.lorawan.pbw.set(myBWs[bw]);
  sprintf(msg, "BW %d, %d KHz: %s", bw, myBWs[bw], rslt ? "ok" : "x");
  Serial.println(msg);
  displayScroll(msg);
  if (!rslt) return false;

  rslt = api.lorawan.pcr.set(cr);
  sprintf(msg, "C/R 4/%d: %s", (cr + 5), rslt ? "ok" : "x");
  Serial.println(msg);
  displayScroll(msg);
  if (!rslt) return false;

  rslt = api.lorawan.ppl.set(preamble);
  sprintf(msg, "preamble %d b: %s", preamble, rslt ? "ok" : "x");
  Serial.println(msg);
  displayScroll(msg);
  if (!rslt) return false;

  rslt = api.lorawan.ptp.set(txPower);
  sprintf(msg, "TX power %d: %s", txPower, rslt ? "ok" : "x");
  Serial.println(msg);
  displayScroll(msg);
  if (!rslt) return false;

  // SX126xWriteRegister(0x08e7, OCP_value);
  // Serial.printf("Set OCP to 0x%2x [%d]\n", OCP_value, (OCP_value * 2.5));
  // LoRa callbacks
  api.lorawan.registerPRecvCallback(recv_cb);
  api.lorawan.registerPSendCallback(send_cb);

  rslt = api.lorawan.precv(65533);
  Serial.printf("Reset Rx mode %s\n", rslt ? "ok" : "x");
  if (!rslt) return false;

  return true;
}

void sendMsg(char* msgToSend) {
  if (isSending) {
    uint8_t count = 10;
    while (isSending && count-- > 0) {
      Serial.write('.');
      delay(500);
    }
    Serial.write('\n');
    if (isSending) {
      Serial.println("isSending strill true after 5 seconds!");
      return;
    }
  }
  isSending = true;
  uint8_t ln = strlen(msgToSend);
  // NO NEED
  // With api.lorawan.precv(65533), you can still can do TX without calling api.lorawan.precv(0).
  // api.lorawan.precv(0);
  // turn off reception – a little hackish, but without that send might fail.
  // memset(msg, 0, ln + 20);
  Serial.printf("Sending `%s`: ", msgToSend);
  bool rslt = api.lorawan.psend(ln, (uint8_t*)msgToSend);
  // when done it will call void send_cb(void);
  Serial.printf("Sending `%s` via P2P: %s\n", msgToSend, rslt ? "ok" : "x");
  sprintf(msg, "Sent %s via P2P:", rslt ? "[o]" : "[x]");
  displayScroll(msg);
  displayScroll(msgToSend);
}

void displayScroll(char *msgToSend) {
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

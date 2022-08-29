extern SSOLED ssoled;
extern bool isSending;
char version[32] = {0};
bool fullBW = true;

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
  api.lorawan.precv(65534);
}

void recv_cb(rui_lora_p2p_recv_t data) {
  uint16_t ln = data.BufferSize;
  char plainText[ln + 1] = {0};
  // hexDump(data.Buffer, ln);
  char buff[92];
  sprintf(buff, "Incoming message, length: %d, RSSI: %d, SNR: %d", data.BufferSize, data.Rssi, data.Snr);
  Serial.println(buff);
  if (hasOLED) {
    sprintf(buff, "%d-byte packet", data.BufferSize);
    oledFill(&ssoled, 1, 1);
    oledDumpBuffer(&ssoled, NULL);
    oledWriteString(&ssoled, 0, 3, 0, "Incoming!", FONT_NORMAL, 0, 1);
    oledWriteString(&ssoled, 0, 0, 1, buff, FONT_NORMAL, 0, 1);
    sprintf(buff, "RSSI: %d", data.Rssi);
    oledWriteString(&ssoled, 0, 0, 2, buff, FONT_NORMAL, 0, 1);
    sprintf(buff, "SNR: %d", data.Snr);
    oledWriteString(&ssoled, 0, 0, 3, buff, FONT_NORMAL, 0, 1);
  }
  if (needAES) {
    int rslt = aes.Process((char*)data.Buffer, ln, myIV, myPWD, 16, plainText, aes.decryptFlag, aes.ecbMode);
    if (rslt < 0) {
      Serial.printf("Error %d in Process ECB Decrypt\n", rslt);
      return;
    }
    // Serial.println("ECB Decrypt:");
  } else {
    memcpy(plainText, data.Buffer, ln);
  }
  // hexDump((uint8_t*)plainText, ln);
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, plainText);
  if (!error) {
    posY = 4;
    JsonObject root = doc.as<JsonObject>();
    // using C++11 syntax (preferred):
    for (JsonPair kv : root) {
      sprintf(buff, " * %s: %s", kv.key().c_str(), kv.value().as<char*>());
      Serial.println(buff);
      if (hasOLED) oledWriteString(&ssoled, 0, 0, posY++, buff + 1, FONT_NORMAL, 0, 1);
    }
    return; // End for JSON messages
  }
  if (hasOLED) {
    // Let's pretty print the message.
    oledSetTextWrap(&ssoled, true);
    oledWriteString(&ssoled, 0, 0, 4, plainText, FONT_NORMAL, 0, 1);
    posY = 6;
  }
  Serial.println("Message:");
  // char msg[data.BufferSize + 1];
  // memset(msg, 0, data.BufferSize + 1);
  // memcpy(msg, data.Buffer, data.BufferSize);
  Serial.println(plainText);
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

  if (fullBW) rslt = api.lorawan.pbw.set(bw); // 3.5.0b and onwards you pass the index
  else rslt = api.lorawan.pbw.set(myBWs[bw]); // Pre-3.5.0b you pass the value
  sprintf(msg, "BW %d, %d KHz: %s", bw, myBWs[bw], rslt ? "ok" : "x");
  Serial.println(msg);
  displayScroll(msg);
  // if (!rslt) return false;

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

  rslt = api.lorawan.precv(65534);
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
      Serial.println("isSending still true after 5 seconds!");
      isSending = false;
      return;
    }
  }
  uint8_t ln = strlen(msgToSend);
  // NO NEED
  // With api.lorawan.precv(65533), you can still can do TX without calling api.lorawan.precv(0).
  // api.lorawan.precv(0);
  // turn off reception – a little hackish, but without that send might fail.
  // memset(msg, 0, ln + 20);
  // Serial.printf("Sending `%s`: ", msgToSend);
  isSending = !api.lorawan.psend(ln, (uint8_t*)msgToSend);
  // when done it will call void send_cb(void);
  Serial.printf("Sending `%s`, len %d via P2P: %s\n", msgToSend, ln, isSending ? "x" : "ok");
  sprintf(msg, "Sent %s via P2P:", isSending ? "[x]" : "[o]");
  displayScroll(msg);
  displayScroll(msgToSend);
  api.lorawan.precv(65534);
}

void sendBlock(uint8_t* msgToSend, uint8_t ln) {
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
  // NO NEED
  // With api.lorawan.precv(65533), you can still can do TX without calling api.lorawan.precv(0).
  // api.lorawan.precv(0);
  // turn off reception – a little hackish, but without that send might fail.
  // memset(msg, 0, ln + 20);
  // Serial.println("Sending:");
  // hexDump(msgToSend, ln);
  bool rslt = api.lorawan.psend(ln, msgToSend);
  // when done it will call void send_cb(void);
  Serial.printf("Sending via P2P: %s\n", rslt ? "ok" : "x");
  sprintf(msg, "Sent %s via P2P:", rslt ? "[o]" : "[x]");
  displayScroll(msg);
}

// ------------- Utilities ------------- //
void hexDump(uint8_t* buf, uint16_t len) {
  // Something similar to the Unix/Linux hexdump -C command
  // Pretty-prints the contents of a buffer, 16 bytes a row
  char alphabet[17] = "0123456789abcdef";
  uint16_t i, index;
  Serial.print(F("   +------------------------------------------------+ +----------------+\n"));
  Serial.print(F("   |.0 .1 .2 .3 .4 .5 .6 .7 .8 .9 .a .b .c .d .e .f | |      ASCII     |\n"));
  for (i = 0; i < len; i += 16) {
    if (i % 128 == 0) Serial.print(F("   +------------------------------------------------+ +----------------+\n"));
    char s[] = "|                                                | |                |\n";
    // pre-formated line. We will replace the spaces with text when appropriate.
    uint8_t ix = 1, iy = 52, j;
    for (j = 0; j < 16; j++) {
      if (i + j < len) {
        uint8_t c = buf[i + j];
        // fastest way to convert a byte to its 2-digit hex equivalent
        s[ix++] = alphabet[(c >> 4) & 0x0F];
        s[ix++] = alphabet[c & 0x0F];
        ix++;
        if (c > 31 && c < 128) s[iy++] = c;
        else s[iy++] = '.'; // display ASCII code 0x20-0x7F or a dot.
      }
    }
    index = i / 16;
    // display line number then the text
    if (i < 256) Serial.write(' ');
    Serial.print(index, HEX); Serial.write('.');
    Serial.print(s);
  }
  Serial.print(F("   +------------------------------------------------+ +----------------+\n"));
}

void hex2array(char *src, uint8_t *dst, size_t sLen) {
  size_t i, n = 0;
  for (i = 0; i < sLen; i += 2) {
    uint8_t x, c;
    c = src[i] - 48;
    if (c > 9) c -= 55;
    x = c << 4;
    c = src[i + 1] - 48;
    if (c > 9) c -= 55;
    dst[n++] = (x + c);
  }
}

void array2hex(uint8_t *buf, size_t sLen, char *x) {
  size_t i, len, n = 0;
  const char *hex = "0123456789abcdef";
  for (i = 0; i < sLen; ++i) {
    x[n++] = hex[(buf[i] >> 4) & 0xF];
    x[n++] = hex[buf[i] & 0xF];
  }
  x[n++] = 0;
  // Serial.println(x);
}

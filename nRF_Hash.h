// This file's part of the nRF52840 BSP
#include "crys_hash.h"

class nRFCrypto_Hash {
  public:
    nRFCrypto_Hash(void);
    bool begin(CRYS_HASH_OperationMode_t mode);
    bool begin(uint32_t mode) {
      return begin ((CRYS_HASH_OperationMode_t) mode);
    }
    bool update(uint8_t data[], size_t size);
    uint8_t end(uint32_t result[16]);
    uint8_t end(uint8_t result[64]) {
      return end((uint32_t*) result);
    }
  private:
    CRYS_HASHUserContext_t _context;
    uint8_t _digest_len;
};

/*
   The MIT License (MIT)

   Copyright (c) 2020 Ha Thach (tinyusb.org) for Adafruit Industries

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
   THE SOFTWARE.
*/

//--------------------------------------------------------------------+
// MACRO TYPEDEF CONSTANT ENUM DECLARATION
//--------------------------------------------------------------------+

// Length of Digest Op mode in bytes
static const uint8_t digest_len_arr[] = {
  CRYS_HASH_SHA1_DIGEST_SIZE_IN_BYTES,
  CRYS_HASH_SHA224_DIGEST_SIZE_IN_BYTES,
  CRYS_HASH_SHA256_DIGEST_SIZE_IN_BYTES,
  CRYS_HASH_SHA384_DIGEST_SIZE_IN_BYTES,
  CRYS_HASH_SHA512_DIGEST_SIZE_IN_BYTES,
  CRYS_HASH_MD5_DIGEST_SIZE_IN_BYTES
};

//------------- IMPLEMENTATION -------------//
nRFCrypto_Hash::nRFCrypto_Hash(void) {
  _digest_len = 0;
}

bool nRFCrypto_Hash::begin(CRYS_HASH_OperationMode_t mode) {
  bool test = CRYS_HASH_Init(&_context, mode);
  if (mode < CRYS_HASH_NumOfModes) _digest_len = digest_len_arr[mode];
  return test;
}

bool nRFCrypto_Hash::update(uint8_t data[], size_t size) {
  return CRYS_HASH_Update(&_context, data, size);
}

uint8_t nRFCrypto_Hash::end(uint32_t result[16]) {
  CRYS_HASH_Finish(&_context, result);
  return _digest_len;
}

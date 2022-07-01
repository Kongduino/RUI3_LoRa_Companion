#include "ssi_aes.h"
#include "sns_silib.h"

class nRFCrypto_AES {
  public:
    nRFCrypto_AES(void);
    bool begin(void);
    void end(void);
    int Process(
      char *msg, uint8_t msgLen, uint8_t *IV, uint8_t *pKey, uint8_t pKeyLen,
      char *retBuf, SaSiAesEncryptMode_t modeFlag, SaSiAesOperationMode_t opMode);
    SaSiAesEncryptMode_t encryptFlag = (SaSiAesEncryptMode_t) 0; // SASI_AES_ENCRYPT
    SaSiAesEncryptMode_t decryptFlag = (SaSiAesEncryptMode_t) 1; // SASI_AES_DECRYPT
    SaSiAesOperationMode_t ecbMode = (SaSiAesOperationMode_t) 0; // SASI_AES_MODE_ECB
    SaSiAesOperationMode_t cbcMode = (SaSiAesOperationMode_t) 1; // SASI_AES_MODE_CBC
    SaSiAesOperationMode_t ctrMode = (SaSiAesOperationMode_t) 3; // SASI_AES_MODE_CTR
    uint8_t blockLen(uint8_t);
  private:
    bool _begun;
    SaSiAesPaddingType_t _paddingType = (SaSiAesPaddingType_t) 0; // SASI_AES_PADDING_NONE
    SaSiAesKeyType_t _userKey = (SaSiAesKeyType_t) 0; // SASI_AES_USER_KEY
};

nRFCrypto_AES::nRFCrypto_AES(void) {
  _begun = false;
}

bool nRFCrypto_AES::begin() {
  if (_begun == true) return true;
  _begun = false;
  int ret = SaSi_LibInit();
  if (ret == SA_SILIB_RET_OK) _begun = true;
  return (ret == SA_SILIB_RET_OK);
}

void nRFCrypto_AES::end() {
  _begun = false;
  SaSi_LibFini();
}

int nRFCrypto_AES::Process(
  char *msg, uint8_t msgLen, uint8_t *IV, uint8_t *pKey, uint8_t pKeyLen,
  char *retBuf, SaSiAesEncryptMode_t modeFlag, SaSiAesOperationMode_t opMode) {
  /*
    msg:    the message you want to encrypt. does not need to be a multiple of 16 bytes.
    msgLen:   its length
    IV:     the IV (16 bytes) for CBC
    pKey:   the key (16/24/32 bytes)
    pKeyLen:  its length
    retBuf:   the return buffer. MUST be a multiple of 16 bytes.
    modeFlag: encryptFlag / decryptFlag
    opMode:   ecbMode / cbcMode / ctrMode
  */
  if (!_begun) return -1;
  int ret = SaSi_LibInit();
  if (ret != SA_SILIB_RET_OK) return -2;
  if (pKeyLen % 8 != 0) return -3;
  if (pKeyLen < 16) return -3;
  if (pKeyLen > 32) return -3;
  SaSiAesUserContext_t pContext;
  SaSiError_t err = SaSi_AesInit(&pContext, modeFlag, opMode, _paddingType);
  SaSiAesUserKeyData_t keyData;
  keyData.pKey = pKey;
  keyData.keySize = pKeyLen;
  err = SaSi_AesSetKey(&pContext, _userKey, &keyData, sizeof(keyData));
  if (err != SASI_OK) return -4;
  if (opMode != ecbMode) {
    // Set the IV or else...
    err = SaSi_AesSetIv(&pContext, IV);
    if (err != SASI_OK) return -7;
  }
  uint8_t cx, ln = msgLen, ptLen;
  ptLen = blockLen(msgLen);
  uint8_t modulo = ptLen % 16;
  if (modulo > 0) modulo = 16 - modulo;
  char pDataIn[ptLen] = {modulo};
  // Padding included!
  memcpy(pDataIn, msg, msgLen);
  size_t dataOutBuffSize;
  memset(retBuf, 0, ptLen);
  /*
    if (ptLen > 16) {
    for (cx = 0; cx < ptLen - 16; cx += 16) {
      err = SaSi_AesBlock(&pContext, (uint8_t *) (pDataIn + cx), 16, (uint8_t *) (retBuf + cx));
      if (err != SASI_OK) return -5;
    }
    err = SaSi_AesFinish(&pContext, (size_t) 16, (uint8_t *) (pDataIn + cx), (size_t) 16, (uint8_t *) (retBuf + cx), &dataOutBuffSize);
    if (err != SASI_OK) return -6;
    } else {
    err = SaSi_AesBlock(&pContext, (uint8_t *) pDataIn, 16, (uint8_t *) retBuf);
    if (err != SASI_OK) return -5;
    err = SaSi_AesFinish(&pContext, (size_t) 0, (uint8_t *) (pDataIn), (size_t) 0, (uint8_t *) (retBuf), &dataOutBuffSize);
    if (err != SASI_OK) return -7;
    }
    return ptLen;
  */
  for (cx = 0; cx < ptLen; cx += 16) {
    err = SaSi_AesBlock(&pContext, (uint8_t *) (pDataIn + cx), 16, (uint8_t *) (retBuf + cx));
    if (err != SASI_OK) return -5;
  }
  err = SaSi_AesFinish(
          &pContext, (size_t) 0, (uint8_t *) (pDataIn + cx),
          (size_t) 0, (uint8_t *) (retBuf + cx), &dataOutBuffSize);
  if (err != SASI_OK) return -6;
  return ptLen;
}

uint8_t nRFCrypto_AES::blockLen(uint8_t msgLen) {
  // takes in msgLen, and returns the necessary length, a multiple of 16.
  // AES requires blocks of 16 bytes to run properly.
  if (msgLen < 16) {
    return 16;
  } else {
    uint8_t modulo = 0, myLen;
    modulo = msgLen % 16;
    if (modulo != 0) {
      uint8_t x = (msgLen / 16);
      myLen = (x + 1) * 16;
      modulo = 16 - modulo;
    } else myLen = msgLen;
    return myLen;
  }
}

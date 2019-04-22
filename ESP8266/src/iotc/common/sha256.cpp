#include <avr/pgmspace.h>
#include <string.h>

#include "sha256.h"

const uint32_t SHA256_K[] PROGMEM = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1,
    0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786,
    0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
    0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
    0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a,
    0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

const uint8_t SHA256_INIT_STATE[] PROGMEM = {
    0x67, 0xe6, 0x09, 0x6a,  // H0
    0x85, 0xae, 0x67, 0xbb,  // H1
    0x72, 0xf3, 0x6e, 0x3c,  // H2
    0x3a, 0xf5, 0x4f, 0xa5,  // H3
    0x7f, 0x52, 0x0e, 0x51,  // H4
    0x8c, 0x68, 0x05, 0x9b,  // H5
    0xab, 0xd9, 0x83, 0x1f,  // H6
    0x19, 0xcd, 0xe0, 0x5b   // H7
};

#define ror32(num, bits) ((num << (32 - bits)) | (num >> bits))

void Sha256::init(void) {
  memcpy_P(state.b, SHA256_INIT_STATE, 32);
  byteCount = 0;
  bufferOffset = 0;
}

void Sha256::hashBlock() {
  uint8_t i;
  uint32_t a, b, c, d, e, f, g, h, t1, t2;

  a = state.w[0];
  b = state.w[1];
  c = state.w[2];
  d = state.w[3];
  e = state.w[4];
  f = state.w[5];
  g = state.w[6];
  h = state.w[7];

  for (i = 0; i < 64; i++) {
    if (i >= 16) {
      t1 = buffer.w[i & 15] + buffer.w[(i - 7) & 15];
      t2 = buffer.w[(i - 2) & 15];
      t1 += ror32(t2, 17) ^ ror32(t2, 19) ^ (t2 >> 10);
      t2 = buffer.w[(i - 15) & 15];
      t1 += ror32(t2, 7) ^ ror32(t2, 18) ^ (t2 >> 3);
      buffer.w[i & 15] = t1;
    }
    t1 = h;
    t1 += ror32(e, 6) ^ ror32(e, 11) ^ ror32(e, 25);  // ∑1(e)
    t1 += g ^ (e & (g ^ f));                          // Ch(e,f,g)
    t1 += pgm_read_dword(SHA256_K + i);               // Ki
    t1 += buffer.w[i & 15];                           // Wi
    t2 = ror32(a, 2) ^ ror32(a, 13) ^ ror32(a, 22);   // ∑0(a)
    t2 += ((b & c) | (a & (b | c)));                  // Maj(a,b,c)
    h = g;
    g = f;
    f = e;
    e = d + t1;
    d = c;
    c = b;
    b = a;
    a = t1 + t2;
  }
  state.w[0] += a;
  state.w[1] += b;
  state.w[2] += c;
  state.w[3] += d;
  state.w[4] += e;
  state.w[5] += f;
  state.w[6] += g;
  state.w[7] += h;
}

void Sha256::push(uint8_t data) {
  buffer.b[bufferOffset ^ 3] = data;
  bufferOffset++;
  if (bufferOffset == BLOCK_LENGTH) {
    hashBlock();
    bufferOffset = 0;
  }
}

#if defined(ARDUINO) && ARDUINO >= 100
size_t Sha256::write(uint8_t data) {
#else
void Sha256::write(uint8_t data) {
#endif
  ++byteCount;
  push(data);
#if defined(ARDUINO) && ARDUINO >= 100
  return 1;
#endif
}

void Sha256::padBlock() {
  // Implement SHA-256 padding (fips180-2 §5.1.1)

  // Pad with 0x80 followed by 0x00 until the end of the block
  push(0x80);
  while (bufferOffset != 56) push(0x00);

  // Append length in the last 8 bytes. We're only using 32 bit lengths, but
  // SHA-2 supports 64 bit lengths so zero pad the top bits
  push(0);
  push(0);
  push(0);
  push(byteCount >> 29);
  push(byteCount >> 21);
  push(byteCount >> 13);
  push(byteCount >> 5);
  push(byteCount << 3);
}

uint8_t* Sha256::result(void) {
  // Pad to complete the last block
  padBlock();

  // Swap byte order back
  for (uint8_t i = 0; i < 8; i++) {
    uint32_t a, b;
    a = state.w[i];
    b = a << 24;
    b |= (a << 8) & 0x00ff0000;
    b |= (a >> 8) & 0x0000ff00;
    b |= a >> 24;
    state.w[i] = b;
  }

  // Return pointer to hash
  return state.b;
}

#define HMAC_IPAD 0x36
#define HMAC_OPAD 0x5c

void Sha256::initHmac(const uint8_t* key, size_t keyLength) {
  memset(keyBuffer, 0, BLOCK_LENGTH);
  if (keyLength > BLOCK_LENGTH) {
    // Hash long keys
    init();
    for (; keyLength--;) write(*key++);
    memcpy(keyBuffer, result(), HASH_LENGTH);
  } else {
    // Block length keys are used as is
    memcpy(keyBuffer, key, keyLength);
  }
  reset();
}

uint8_t* Sha256::resultHmac(void) {
  uint8_t i;
  // Complete inner hash
  memcpy(innerHash, result(), HASH_LENGTH);
  // Calculate outer hash
  init();
  for (i = 0; i < BLOCK_LENGTH; i++) write(keyBuffer[i] ^ HMAC_OPAD);
  for (i = 0; i < HASH_LENGTH; i++) write(innerHash[i]);
  return result();
}

void Sha256::reset(void) {
  // Start inner hash
  init();
  for (uint8_t i = 0; i < BLOCK_LENGTH; i++) {
    write(keyBuffer[i] ^ HMAC_IPAD);
  }
}
/* SHA-256 implementation adapted from Brad Conte's crypto-algorithms.
 * https://github.com/B-Con/crypto-algorithms
 * Original code is presented "as is" without any guarantees.
 */
#pragma once

#include <stddef.h>
#include <stdint.h>

namespace XBMC
{
struct SHA256Context
{
  unsigned char data[64];
  uint32_t datalen;
  uint64_t bitlen;
  uint32_t state[8];
};

void SHA256Init(SHA256Context* context);
void SHA256Update(SHA256Context* context, const unsigned char* data, size_t size);
void SHA256Final(SHA256Context* context, unsigned char digest[32]);
}

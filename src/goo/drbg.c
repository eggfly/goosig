/*!
 * drbg.c - hmac-drbg for C89
 * Copyright (c) 2018-2019, Christopher Jeffrey (MIT License).
 * https://github.com/handshake-org/goosig
 *
 * Resources:
 *   https://tools.ietf.org/html/rfc6979
 *   https://csrc.nist.gov/publications/detail/sp/800-90a/archive/2012-01-23
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "drbg.h"

static const unsigned char ZERO[1] = {0x00};
static const unsigned char ONE[1] = {0x01};

static void
goo_drbg_update(goo_drbg_t *drbg, const unsigned char *seed, size_t seed_len);

void
goo_drbg_init(goo_drbg_t *drbg, const unsigned char *seed, size_t seed_len) {
  size_t i;

  assert(seed != NULL);
  assert(seed_len >= 24);

  for (i = 0; i < GOO_SHA256_HASH_SIZE; i++) {
    drbg->K[i] = 0x00;
    drbg->V[i] = 0x01;
  }

  goo_drbg_update(drbg, seed, seed_len);
}

static void
goo_drbg_update(goo_drbg_t *drbg, const unsigned char *seed, size_t seed_len) {
  goo_hmac_init(&drbg->kmac, &drbg->K[0], GOO_SHA256_HASH_SIZE);
  goo_hmac_update(&drbg->kmac, &drbg->V[0], GOO_SHA256_HASH_SIZE);
  goo_hmac_update(&drbg->kmac, &ZERO[0], 1);

  if (seed != NULL)
    goo_hmac_update(&drbg->kmac, seed, seed_len);

  goo_hmac_final(&drbg->kmac, &drbg->K[0]);

  goo_hmac_init(&drbg->kmac, &drbg->K[0], GOO_SHA256_HASH_SIZE);
  goo_hmac_update(&drbg->kmac, &drbg->V[0], GOO_SHA256_HASH_SIZE);
  goo_hmac_final(&drbg->kmac, &drbg->V[0]);

  if (seed != NULL) {
    goo_hmac_init(&drbg->kmac, &drbg->K[0], GOO_SHA256_HASH_SIZE);
    goo_hmac_update(&drbg->kmac, &drbg->V[0], GOO_SHA256_HASH_SIZE);
    goo_hmac_update(&drbg->kmac, &ONE[0], 1);
    goo_hmac_update(&drbg->kmac, seed, seed_len);
    goo_hmac_final(&drbg->kmac, &drbg->K[0]);

    goo_hmac_init(&drbg->kmac, &drbg->K[0], GOO_SHA256_HASH_SIZE);
    goo_hmac_update(&drbg->kmac, &drbg->V[0], GOO_SHA256_HASH_SIZE);
    goo_hmac_final(&drbg->kmac, &drbg->V[0]);
  }
}

void
goo_drbg_generate(goo_drbg_t *drbg, void *out, size_t len) {
  unsigned char *bytes = (unsigned char *)out;
  size_t pos = 0;
  size_t left = len;
  size_t outlen = GOO_SHA256_HASH_SIZE;

  while (pos < len) {
    goo_hmac_init(&drbg->kmac, &drbg->K[0], GOO_SHA256_HASH_SIZE);
    goo_hmac_update(&drbg->kmac, &drbg->V[0], GOO_SHA256_HASH_SIZE);
    goo_hmac_final(&drbg->kmac, &drbg->V[0]);

    if (outlen > left)
      outlen = left;

    memcpy(bytes + pos, &drbg->V[0], outlen);

    pos += outlen;
    left -= outlen;
  }

  assert(pos == len);
  assert(left == 0);

  goo_drbg_update(drbg, NULL, 0);
}

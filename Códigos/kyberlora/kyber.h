#pragma once
#include "config.h"

// Primitivas expostas ao .ino
void kyber_keygen(PublicKey &pk, SecretKey &sk,
                  uint32_t sa, uint32_t ssk);
void kyber_encrypt(Ciphertext &ct, const PublicKey &pk,
                   const uint8_t *bits, int nbits, uint32_t sr);
void kyber_decrypt(uint8_t *bits_out, int nbits,
                   const SecretKey &sk, const Ciphertext &ct);

// Utilitários de bits/bytes
void bytes_para_bits(uint8_t *bits, const uint8_t *data, int nbytes);
void bits_para_bytes(uint8_t *data, const uint8_t *bits, int nbytes);

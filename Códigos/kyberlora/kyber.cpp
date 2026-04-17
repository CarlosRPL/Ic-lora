#include "kyber.h"
#include <string.h>

// Buffers internos (escopo de arquivo — não vazam para fora)
static PolyMatrix A;
static PolyVec s, e0, As, r, e1, ATr, u, u_dec;
static Poly e2, tr, msg_poly, v_raw, v_dec, su, rec;

// ── PRNG ─────────────────────────────────────────────────────────────────────
static uint32_t rotl(uint32_t x, int r) { return (x << r) | (x >> (32 - r)); }
static uint32_t derive(uint32_t seed, uint32_t idx) {
  uint32_t x = seed, y = idx, z = 0x9E3779B9, w = 0xBB67AE85;
  for (int i = 0; i < 4; i++) {
    x += y; w ^= x; w = rotl(w,16);
    z += w; y ^= z; y = rotl(y,12);
    x += y; w ^= x; w = rotl(w, 8);
    z += w; y ^= z; y = rotl(y, 7);
  }
  return x ^ y ^ z ^ w;
}

// ── Polinômios ────────────────────────────────────────────────────────────────
static int16_t mod_q(int32_t x) {
  int16_t r = (int16_t)(x % Q);
  return r < 0 ? r + Q : r;
}
static void poly_add(Poly r, const Poly a, const Poly b) {
  for (int i = 0; i < N; i++) r[i] = mod_q((int32_t)a[i] + b[i]);
}
static void poly_sub(Poly r, const Poly a, const Poly b) {
  for (int i = 0; i < N; i++) r[i] = mod_q((int32_t)a[i] - b[i]);
}
static void poly_mul(Poly r, const Poly a, const Poly b) {
  int32_t tmp[N] = {0};
  for (int i = 0; i < N; i++)
    for (int j = 0; j < N; j++) {
      int idx = i + j;
      int32_t prod = (int32_t)a[i] * b[j];
      if (idx >= N) tmp[idx - N] -= prod;
      else          tmp[idx]     += prod;
    }
  for (int i = 0; i < N; i++) r[i] = mod_q(tmp[i]);
}
static void polyvec_inner(Poly res, const PolyVec a, const PolyVec b) {
  Poly acc = {}, tmp;
  for (int i = 0; i < K; i++) { poly_mul(tmp, a[i], b[i]); poly_add(acc, acc, tmp); }
  memcpy(res, acc, sizeof(Poly));
}
static void matrix_vec_mul(PolyVec res, const PolyMatrix M, const PolyVec v) {
  for (int i = 0; i < K; i++) polyvec_inner(res[i], M[i], v);
}
static void matrix_T_vec_mul(PolyVec res, const PolyMatrix M, const PolyVec v) {
  for (int i = 0; i < K; i++) {
    PolyVec col;
    for (int j = 0; j < K; j++) memcpy(col[j], M[j][i], sizeof(Poly));
    polyvec_inner(res[i], col, v);
  }
}
static void generate_A(PolyMatrix A, uint32_t seed) {
  for (int i = 0; i < K; i++)
    for (int j = 0; j < K; j++) {
      uint32_t s = derive(seed, (uint32_t)(i*100+j));
      for (int c = 0; c < N; c++) { s = derive(s,c); A[i][j][c] = (int16_t)(s % Q); }
    }
}
static void sample_small(Poly out, uint32_t seed, uint32_t idx) {
  uint32_t s = derive(seed, idx * 999);
  for (int c = 0; c < N; c++) {
    s = derive(s, c);
    int coef = __builtin_popcount((s >> 8) & 0xFF) - __builtin_popcount(s & 0xFF);
    if (coef >  ETA) coef =  ETA;
    if (coef < -ETA) coef = -ETA;
    out[c] = mod_q(coef);
  }
}
static void sample_small_vec(PolyVec out, uint32_t seed, uint32_t base) {
  for (int i = 0; i < K; i++) sample_small(out[i], seed, base + i);
}

// ── Compressão ────────────────────────────────────────────────────────────────
static void compress_poly(int16_t out[N], const Poly p, int d) {
  for (int i = 0; i < N; i++)
    out[i] = (int16_t)(((int32_t)p[i] << d) + Q/2) / Q & ((1 << d) - 1);
}
static void decompress_poly(Poly out, const int16_t in[N], int d) {
  for (int i = 0; i < N; i++)
    out[i] = (int16_t)(((int32_t)in[i] * Q) + (1 << (d-1))) >> d;
}

// ── Codificação ───────────────────────────────────────────────────────────────
static void encode_message(Poly out, const uint8_t *bits, int nbits) {
  int meio = Q / 2;
  memset(out, 0, sizeof(Poly));
  for (int i = 0; i < N && i < nbits; i++) out[i] = bits[i] ? (int16_t)meio : 0;
}
static void decode_message(uint8_t *bits, const Poly p, int nbits) {
  for (int i = 0; i < nbits && i < N; i++)
    bits[i] = (mod_q(p[i]) > Q/4 && mod_q(p[i]) < 3*Q/4) ? 1 : 0;
}

// ── API pública ───────────────────────────────────────────────────────────────
void bytes_para_bits(uint8_t *bits, const uint8_t *data, int nbytes) {
  for (int i = 0; i < nbytes; i++)
    for (int b = 7; b >= 0; b--) bits[i*8+(7-b)] = (data[i] >> b) & 1;
}
void bits_para_bytes(uint8_t *data, const uint8_t *bits, int nbytes) {
  for (int i = 0; i < nbytes; i++) {
    data[i] = 0;
    for (int b = 0; b < 8; b++) data[i] = (data[i] << 1) | bits[i*8+b];
  }
}
void kyber_keygen(PublicKey &pk, SecretKey &sk, uint32_t sa, uint32_t ssk) {
  generate_A(A, sa);
  sample_small_vec(s,  ssk, 0);
  sample_small_vec(e0, ssk ^ 0xDEAD, 0);
  matrix_vec_mul(As, A, s);
  for (int i = 0; i < K; i++) poly_add(pk.t[i], As[i], e0[i]);
  pk.semente = sa;
  memcpy(sk.s, s, sizeof(PolyVec));
}
void kyber_encrypt(Ciphertext &ct, const PublicKey &pk,
                   const uint8_t *bits, int nbits, uint32_t sr) {
  generate_A(A, pk.semente);
  sample_small_vec(r,  sr, 0);
  sample_small_vec(e1, sr ^ 0xBEEF, 0);
  sample_small    (e2, sr ^ 0xCAFE, 0);
  matrix_T_vec_mul(ATr, A, r);
  for (int i = 0; i < K; i++) poly_add(u[i], ATr[i], e1[i]);
  polyvec_inner(tr, pk.t, r);
  encode_message(msg_poly, bits, nbits);
  poly_add(v_raw, tr, e2);
  poly_add(v_raw, v_raw, msg_poly);
  for (int i = 0; i < K; i++) compress_poly(ct.u[i], u[i], DU);
  compress_poly(ct.v, v_raw, DV);
}
void kyber_decrypt(uint8_t *bits_out, int nbits,
                   const SecretKey &sk, const Ciphertext &ct) {
  for (int i = 0; i < K; i++) decompress_poly(u_dec[i], ct.u[i], DU);
  decompress_poly(v_dec, ct.v, DV);
  polyvec_inner(su, sk.s, u_dec);
  poly_sub(rec, v_dec, su);
  decode_message(bits_out, rec, nbits);
}

#pragma once
#include <stdint.h>

//Nó 
#define MASTER 1   // 0 no segundo nó

//Kyber-512 
#define N   256
#define K   2
#define Q   3329
#define ETA 3
#define DU  10
#define DV  4

typedef int16_t Poly[N];
typedef Poly    PolyVec[K];
typedef PolyVec PolyMatrix[K];

struct PublicKey  { uint32_t semente; PolyVec t; };
struct SecretKey  { PolyVec s; };
struct Ciphertext { int16_t u[K][N]; int16_t v[N]; };

//Hardware 
#define SS_PIN    18
#define RST_PIN   14
#define DIO0_PIN  26
#define FREQ      915E6
#define SF        9
#define TX_POWER  20
#define BANDWIDTH 125E3

#define OLED_ADDR 0x3C
#define SDA_PIN   4
#define SCL_PIN   15

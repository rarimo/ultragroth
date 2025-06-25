#ifndef KECCAK_H
#define KECCAK_H

//#include <QtGlobal>

// ORIGIN: https://github.com/gvanas/KeccakCodePackage

typedef unsigned char u8;
typedef unsigned long long int u64;
typedef unsigned int ui;

void Keccak(ui r, ui c, const u8 *in, u64 inLen, u8 sfx, u8 *out, u64 outLen);
void FIPS202_KECCAK_256(const u8 *in, u64 inLen, u8 *out);

int LFSR86540(u8 *R);
#define ROL(a,o) ((((u64)a)<<o)^(((u64)a)>>(64-o)))
static u64 load64(const u8 *x);
static void store64(u8 *x, u64 u);
static void xor64(u8 *x, u64 u);
void KeccakF1600(void *s);
void Keccak(ui r, ui c, const u8 *in, u64 inLen, u8 sfx, u8 *out, u64 outLen);

#endif // KECCAK_H
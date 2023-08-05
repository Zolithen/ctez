#ifndef TYPES_H_INCLUDED
#define TYPES_H_INCLUDED

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long u64;

typedef char i8;
typedef short i16;
typedef int i32;
typedef long i64;

#define IS_FLAG_ON(f1, f2) (f1 & f2) == f2
#define SET_FLAG_ON(f1, f2) f1 |= f2
#define SET_FLAG_OFF(f1, f2) f1 &= 255 ^ f2

#endif // TYPES_H_INCLUDED

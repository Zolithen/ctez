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

// Project specific
#define BOTTOM_SECTION_HEIGHT 12
#define MAX_WINDOWS 7
#define TWINCOM 0
#define TWINFILE 1
#define TWINCOMINPUT 2
#define TWIN1 MAX_WINDOWS - 1
#define TWIN2 MAX_WINDOWS - 2
#define TWIN3 MAX_WINDOWS - 3
#define TWIN4 MAX_WINDOWS - 4
#define TWIN(n) MAX_WINDOWS - n

#endif // TYPES_H_INCLUDED

#ifndef MISC_H_INCLUDED
#define MISC_H_INCLUDED

#include <stdlib.h>
#include "databuffer.h"

void* ecalloc(size_t n, size_t size);
void* emalloc(size_t size);
void* erealloc(void* block, size_t size);
int   mini(int a, int b); // i indicates integer
int   maxi(int a, int b);

wchar_t* wstrcat(const wchar_t* first, const wchar_t* second, int szfirst, int szsecond); /* Concatenates 2 wchar_t strings given their sizes */

char*    utftrans_16to8(wchar_t* str);
Data_buffer* utftrans_8to16(unsigned char* str, int maxsize);

#endif // MISC_H_INCLUDED

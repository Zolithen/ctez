#ifndef MISC_H_INCLUDED
#define MISC_H_INCLUDED

#include <stdlib.h>
#include <stdbool.h>
#include "databuffer.h"

//#define MEMORY_LOG 1


void* debug_calloc(size_t n, size_t size, const char* name, int line);
void* debug_malloc(size_t size, const char* name, int line);
void* debug_realloc(void* block, size_t size, const char* name, int line);
void debug_free(void* block, const char* name, int line);

#ifdef MEMORY_LOG
#define ecalloc(n, size) debug_calloc(n, size, __FILE__, __LINE__)
#define emalloc(size) debug_malloc(size, __FILE__, __LINE__)
#define erealloc(block, size) debug_realloc(block, size, __FILE__, __LINE__)
#define free(block) debug_free(block, __FILE__, __LINE__)
#else
void* ecalloc(size_t n, size_t size);
void* emalloc(size_t size);
void* erealloc(void* block, size_t size);
void efree(void* block);
#endif
int   mini(int a, int b); // i indicates integer
int   maxi(int a, int b);

wchar_t* wstrcat_in_tbuffer_render(const wchar_t* first, const wchar_t* second, int szfirst, int szsecond);
wchar_t* wstrcat(const wchar_t* first, const wchar_t* second, int szfirst, int szsecond, int* ressize); /* Concatenates 2 wchar_t strings given their sizes */
bool     wstrisnum(const wchar_t* str, int sz); /* Returns true if str represents a number */
int      wstrtonum(const wchar_t* str, int sz); /* Turn a wide string into a number */
bool     wstrcmp(const wchar_t* first, const wchar_t* second, int szfirst, int szsecond);
u8*      wstrdgr(const wchar_t* str, int sz); /* Turns a wstr (that only consists of ASCII chars) into a normal string. String char size is the same */

Data_buffer* utftrans_16to8(wchar_t* str, int maxsize);
Data_buffer* utftrans_8to16(u8* str, int maxsize);

#endif // MISC_H_INCLUDED

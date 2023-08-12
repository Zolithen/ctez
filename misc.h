#ifndef MISC_H_INCLUDED
#define MISC_H_INCLUDED

#include <stdlib.h>
#include <stdbool.h>

#include "types.h"
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

Data_buffer* utftrans_16to8(wchar_t* str, int maxsize);
Data_buffer* utftrans_8to16(u8* str, int maxsize);

#endif // MISC_H_INCLUDED

#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>

#include "misc.h"

#include "databuffer.h"

void* debug_calloc(size_t n, size_t size, const char* name, int line) {
    void* res = calloc(n, size);
    if (res == NULL) {
        exit(-1);
    }
    printf("Allocated %d bytes in %p on %s L:%d\n", (int)(n*size), res, name, line);
    return res;
}

void* debug_malloc(size_t size, const char* name, int line) {
    void* result = malloc(size);
	if (result == NULL) {
        exit(-1);
	}
	printf("Allocated %d bytes in %p on %s L:%d\n", (int)size, result, name, line);
	return result;
}
void* debug_realloc(void* block, size_t size, const char* name, int line) {
    void* result = realloc(block, size);
	if (result == NULL) {
        exit(-1);
	}
	printf("Reallocated block %p to %p with %d bytes in %s L:%d\n", block, result, (int)size, name, line);
	return result;
}
void debug_free(void* block, const char* name, int line) {
    //free(block);
    printf("Freed block %p on %s L:%d\n", block, name, line);
}

#ifndef MEMORY_LOG
void* ecalloc(size_t n, size_t size) {
    void* result = calloc(n, size);
	if (result == NULL) {
        exit(-1);
	}
	return result;
}

void* emalloc(size_t size) {
	void* result = malloc(size);
	if (result == NULL) {
        exit(-1);
	}
	return result;
}

void* erealloc(void* block, size_t size) {
	void* result = realloc(block, size);
	if (result == NULL) {
        exit(-1);
	}
	return result;
}

void efree(void* block) {
    free(block);
}
#endif



int mini(int a, int b) {
    return a > b ? b : a;
}
int maxi(int a, int b) {
    return a > b ? a : b;
}

/* Description of the algorithm:
   https://en.wikipedia.org/wiki/UTF-8#Encoding
   Remember that every possible character in 0x0000 - 0xFFFF is the same in UTF16,
   the only range where this doesn't apply (0xD800 - 0xDFFF) are used to signal that
   a codepoint uses 2 16-words, which is outside of the range the Windows console default font.
   So, a character in UTF16 under our considerations doesn't need any processing to get it's
   UTF32 translation.*/
Data_buffer* utftrans_16to8(wchar_t* str, int maxsize) {
    Data_buffer* dat = databuffer_new(10);
    for (int i = 0; i < maxsize; i++) {
        u16 c = str[i];
        // We first get how many bytes we need to encode a certain codepoint
        if (c <= 0x007F) { // 1 byte
            databuffer_add_byte(dat, (u8)c);
        } else if (c >= 0x0080 && c <= 0x07FF) { // 2 bytes
            u8 first = (u8) ((c & 0x07C0) >> 6);
            u8 second = (u8) (c & 0x003F);
            databuffer_add_byte(dat, 0b11000000 | first);
            databuffer_add_byte(dat, 0b10000000 | second);
        } else if (c >= 0x0800 ) { // 3 bytes
            u8 first = (u8) ((c & 0xF000) >> 12);
            u8 second = (u8) ((c & 0x0FC0) >> 6);
            u8 third = (u8) (c & 0x003F);

            databuffer_add_byte(dat, 0b11100000 | first);
            databuffer_add_byte(dat, 0b10000000 | second);
            databuffer_add_byte(dat, 0b10000000 | third);
        }
    }
    return dat;
}

/* We only need to implement up to 3 bytes because 4 bytes are out of the windows console font.
   We need to properly report if the string has a character that needs 4 bytes to see if it's
   supported or not to not corrupt files. The method returns a Data_buffer which data, when casted
   to an ungisned short* is the UTF16 string.
   Description of the algorithm:
   https://stackoverflow.com/questions/73758747/looking-for-the-description-of-the-algorithm-to-convert-utf8-to-utf16 */
Data_buffer* utftrans_8to16(u8* str, int maxsize) {
    Data_buffer* dat = databuffer_new(10);
    for (int i = 0; i < maxsize; i++) {
        u8 c = str[i];

        // Check how many bytes the next character occupies
        if ((c & 0x80) == 0x00) { // 1 byte
            u16 utf16 = c & 0x7F;

            /*unsigned char* a = (unsigned char*)&utf16; // Programming war crimes. We split the short into 2 chars to save in the databuffer
            databuffer_add_byte(dat, *a);
            databuffer_add_byte(dat, *(a+1));*/
            databuffer_add_bytes(dat, (u8*)&utf16, 2);
        } else if ((c & 0xE0) == 0xC0) { // 2 bytes
            u8 secbyte = str[i+1];
            if (secbyte & 0xC0) { // Ensure we are parsing correct UTF8
                u16 utf16 = ((c & 0x1F) << 6) | (secbyte & 0x3F);

                databuffer_add_bytes(dat, (u8* )&utf16, 2);
            }
            i++;
        } else if ((c & 0xF0) == 0xE0) { // 3 bytes
            u8 secbyte = str[i+1];
            u8 thdbyte = str[i+2];
            if((secbyte & 0xC0) && (thdbyte & 0xC0)) {
                u16 utf16 = ((c & 0x0F) << 12) | ((secbyte & 0x3F) << 6) | (thdbyte & 0x3F);

                databuffer_add_bytes(dat, (u8* )&utf16, 2);
            }
            i+=2;
        } else if ((c & 0xF8) == 0xF0) { // Throw error. UTF8 representation has 4 bytes and thus not supported. TODO: better error report (ej: not opening file)
            assert(false);
        } else { // TODO: don't
            return dat;
        }
    }

    return dat;
}

#include "misc.h"
#include "databuffer.h"
#include <assert.h>
#include <string.h>
#include <stdbool.h>

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

int mini(int a, int b) {
    return a > b ? b : a;
}
int maxi(int a, int b) {
    return a > b ? a : b;
}

wchar_t* wstrcat(const wchar_t* first, const wchar_t* second, int szfirst, int szsecond) {
    wchar_t* res = ecalloc(szfirst + szsecond + 1, sizeof(wchar_t));
    memcpy(res, first, szfirst*sizeof(wchar_t));
    memcpy(res+szfirst, second, szsecond*sizeof(wchar_t));
    res[szfirst + szsecond] = 0;
    return res;
}

char* utftrans_16to8(wchar_t* str);

/* We only need to implement up to 3 bytes because 4 bytes are out of the windows console font.
   We need to properly report if the string has a character that needs 4 bytes to see if it's
   supported or not to not corrupt files. The method returns a Data_buffer which data, when casted
   to an ungisned short* is the UTF16 string.
   Description of the algorithm:
   https://stackoverflow.com/questions/73758747/looking-for-the-description-of-the-algorithm-to-convert-utf8-to-utf16 */
Data_buffer* utftrans_8to16(unsigned char* str, int maxsize) {
    Data_buffer* dat = databuffer_new(10);
    for (int i = 0; i < maxsize; i++) {
        unsigned char c = str[i];

        // Check how many bytes the next character occupies
        if ((c & 0x80) == 0x00) { // 1 byte
            unsigned short utf16 = c & 0x7F;

            /*unsigned char* a = (unsigned char*)&utf16; // Programming war crimes. We split the short into 2 chars to save in the databuffer
            databuffer_add_byte(dat, *a);
            databuffer_add_byte(dat, *(a+1));*/
            databuffer_add_bytes(dat, (unsigned char* )&utf16, 2);
        } else if ((c & 0xE0) == 0xC0) { // 2 bytes
            unsigned char secbyte = str[i+1];
            if (secbyte & 0xC0) { // Ensure we are parsing correct UTF8
                unsigned short utf16 = ((c & 0x1F) << 6) | (secbyte & 0x3F);

                databuffer_add_bytes(dat, (unsigned char* )&utf16, 2);
            }
            i++;
        } else if ((c & 0xF0) == 0xE0) { // 3 bytes
            unsigned char secbyte = str[i+1];
            unsigned char thdbyte = str[i+2];
            if((secbyte & 0xC0) && (thdbyte & 0xC0)) {
                unsigned short utf16 = ((c & 0x0F) << 12) | ((secbyte & 0x3F) << 6) | (thdbyte & 0x3F);

                databuffer_add_bytes(dat, (unsigned char* )&utf16, 2);
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

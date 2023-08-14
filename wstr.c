#include <string.h>
#include <math.h>
#include <stdio.h>

#include "wstr.h"

#include "types.h"
#include "misc.h"

void wstr_start() {
    GNSTRINIT(STR_TEMPORAL_FILE_EXTENSION, (u8*)".longextensionsothatnofilesareoverwrittenctez");

    GSTRINIT(STR_SYS_NEW_BUFFER, L"New buffer");
    GSTRINIT(STR_BUFFER_NOT_LINKED_TO_FILE, L"Buffer has not been linked to a file\n");

    GSTRINIT(STR_COMMSG_NEED_EXACTARGS, L"Command needs exactly %d arguments\n");
    GSTRINIT(STR_COMMSG_INVALID_COMMAND, L"Invalid command\n");
    GSTRINIT(STR_COMMSG_INVALID_ARGTYPE, L"Invalid argument type\n");
    GSTRINIT(STR_COMMSG_INVALID_ARGRANGE, L"Argument out of range\n");
    GSTRINIT(STR_COMMSG_INVALID_PATH, L"Invalid path\n");
    GSTRINIT(STR_COMMSG_BUFFER_ALREADY_BOUND, L"Buffer is already bound to window %d\n");

    GSTRINIT(STR_COMMSG_SAVED_FILE, L"File has been saved\n");
}

bool wchrissymbol(wchar_t c) {
    return ((c == L' ') ||
            (c == L'(') ||
            (c == L')') ||
            (c == L'{') ||
            (c == L'}') ||
            (c == L'\'') ||
            (c == L'"') ||
            (c == L'+') ||
            (c == L'-') ||
            (c == L'*') ||
            (c == L'/') ||
            (c == L'\\') ||
            (c == L'%') ||
            (c == L'^') ||
            (c == L'&') ||
            (c == L'!') ||
            (c == L'|') ||
            (c == L',') ||
            (c == L'.') ||
            (c == L'=') ||
            (c == L'[') ||
            (c == L']') ||
            (c == L';')
    );
}

/*
    Wide string functions
*/

// TODO: ensure this works correctly
/* This function for some reason doesn't have the convention of sizes containing the terminator, because it's used in a place of strings without terminators */
wchar_t* wstrcat_in_tbuffer_render(const wchar_t* first, const wchar_t* second, int szfirst, int szsecond) {
    wchar_t* res = ecalloc(szfirst + szsecond + 1, sizeof(wchar_t));
    memcpy(res, first, szfirst*sizeof(wchar_t));
    memcpy(res+szfirst, second, szsecond*sizeof(wchar_t));
    res[szfirst + szsecond] = 0;
    return res;
}

/* Concatenates 2 wchar_t strings given their sizes */
wchar_t* wstrcat(const wchar_t* first, const wchar_t* second, u32 szfirst, u32 szsecond, u32* ressize) {
    wchar_t* res = ecalloc(szfirst + szsecond - 1, sizeof(wchar_t));
    memcpy(res, first, (szfirst-1)*sizeof(wchar_t) );
    memcpy(res+szfirst - 1, second, szsecond*sizeof(wchar_t));
    if (ressize != NULL) *ressize = szfirst + szsecond - 1;
    return res;
}

wchar_t* wstrcat_second_no_terminator(const wchar_t* first, const wchar_t* second, u32 szfirst, u32 szsecond, u32* ressize) {
    wchar_t* res = ecalloc(szfirst + szsecond, sizeof(wchar_t));
    memcpy(res, first, (szfirst-1)*sizeof(wchar_t) );
    memcpy(res+szfirst - 1, second, szsecond*sizeof(wchar_t));
    if (ressize != NULL) *ressize = szfirst + szsecond;
    res[szfirst + szsecond - 1] = '\0';
    return res;
}

wchar_t* wstrfromnum(u32 num, u32* ressize) {
    u32 sz = ((u32) floor(log10(num)) + 1);
    wchar_t* text = ecalloc(sz + 1, sizeof(wchar_t));

    u32 prevpower = 1;
    for (u32 i = 0; i < sz; i++) {
        u8 digit = ((num % (u32)pow(10, i + 1)) - (num%prevpower))/(prevpower);
        text[sz - i - 1] = int_to_wstr_char_array[digit];
        prevpower = (u32) pow(10, i + 1);
    }

    if (ressize != NULL) *ressize = sz + 1;

    return text;
}

wchar_t* wstrfilefrompath(const wchar_t* str, u32 sz, u32* ressize) {
    const wchar_t* secstr = str + sz - 1;
    while ((secstr > str) && (*secstr != L'/') && (*secstr != L'\\')) secstr--;
    if ((secstr != str) || (*secstr == L'/') || (*secstr == L'\\')) secstr++;
    *ressize = str + sz - secstr;
    wchar_t* newstr = ecalloc(*ressize, sizeof(wchar_t));
    memcpy(newstr, secstr, (*ressize)*sizeof(wchar_t));
    return newstr;
}

wchar_t* wstrformat(int* ressize, const wchar_t* formatstring, ...) {
    va_list args;
    va_start(args, formatstring);

    int chars_to_alloc = _vscwprintf(formatstring, args) + 1; // _vscwprintf doesn't count the terminating char
    if (ressize != NULL) *ressize = chars_to_alloc;
    wchar_t* res = emalloc(chars_to_alloc * sizeof(wchar_t));
    _vsnwprintf_s(res, chars_to_alloc, chars_to_alloc - 1, formatstring, args);

    va_end(args);
    return res;
}

bool wstrisnum(const wchar_t* str, u32 sz) {
    for (u32 i = 0; i < sz - 1; i++) { // This can be done with a while loop
        if ((str[i] < '0') || (str[i] > '9')) return false;
    }
    return true;
}

u32 wstrtonum(const wchar_t* str, u32 sz) {
    u32 acum = 0;
    u32 power = 0;
    for (int i = (int)sz - 2; i >= 0; i--) { // Do not change i to u32
        acum += (str[i] - '0')*pow(10, power);
        power++;
    }
    return acum;
}

bool wstrcmp(const wchar_t* first, const wchar_t* second, u32 szfirst, u32 szsecond) {
    if (szfirst == szsecond) {
        for (u32 i = 0; i < szfirst; i++) {
            if (first[i] != second[i]) return false;
        }

        return true;
    }

    return false;
}

u8* wstrdgr(const wchar_t* str, u32 sz) {
    const wchar_t* p = str;
    u8* cur = (u8*)p;
    u8* res = ecalloc(sz, sizeof(u8));
    for (u32 i = 0; i < sz; i++) res[i] = cur[2*i]; // TODO: maybe change to a cast
    return res;
}

u32 wstrlen(const wchar_t* str) {
    const wchar_t* t = str;
    while (*t != L'\0') t++;
    return t - str;
}


/*
    Narrow string functions
*/

u8* nstrfilefrompath(const u8* str, u32 sz, u32* ressize) { // Literally the same as the wstr variant but without wchar_t & L's
    const u8* secstr = str + sz - 1;
    while ((secstr > str) && (*secstr != '/') && (*secstr != '\\')) secstr--;
    if ((secstr != str) || (*secstr == '/') || (*secstr == '\\')) secstr++;
    *ressize = str + sz - secstr;
    u8* newstr = ecalloc(*ressize, sizeof(u8));
    memcpy(newstr, secstr, (*ressize)*sizeof(u8));
    return newstr;
}

u32 nstrlen(const u8* str) {
    const u8* t = str;
    while (*t != '\0') t++;
    return t - str;
}

u8* nstrcat(const u8* first, const u8* second, u32 szfirst, u32 szsecond, u32* ressize) {
    u8* res = ecalloc(szfirst + szsecond - 1, sizeof(u8));
    memcpy(res, first, (szfirst-1)*sizeof(u8) );
    memcpy(res+szfirst - 1, second, szsecond*sizeof(u8));
    if (ressize != NULL) *ressize = szfirst + szsecond - 1;
    return res;
}

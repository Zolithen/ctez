#include <string.h>
#include <math.h>

#include "wstr.h"

#include "types.h"
#include "misc.h"

void wstr_start() {
    GSTRINIT(STR_COMMSG_NEED_EXACTARGS, L"Command needs exactly %d arguments\n");
    GSTRINIT(STR_COMMSG_INVALID, L"Invalid command\n");
    GSTRINIT(STR_COMMSG_INVALID_ARGTYPE, L"Invalid argument type\n");
    GSTRINIT(STR_COMMSG_INVALID_ARGRANGE, L"Argument out of range\n");
}

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
wchar_t* wstrcat(const wchar_t* first, const wchar_t* second, int szfirst, int szsecond, int* ressize) {
    wchar_t* res = ecalloc(szfirst + szsecond - 1, sizeof(wchar_t));
    memcpy(res, first, (szfirst-1)*sizeof(wchar_t) );
    memcpy(res+szfirst - 1, second, szsecond*sizeof(wchar_t));
    if (ressize != NULL) *ressize = szfirst + szsecond - 1;
    return res;
}

bool wstrisnum(const wchar_t* str, int sz) {
    for (int i = 0; i < sz - 1; i++) {
        if ((str[i] < '0') || (str[i] > '9')) return false;
    }
    return true;
}

u32 wstrtonum(const wchar_t* str, u32 sz) {
    u32 acum = 0;
    u32 power = 0;
    for (int i = (int)sz - 2; i >= 0; i--) {
        acum += (str[i] - '0')*pow(10, power);
        power++;
    }
    return acum;
}

wchar_t* wstrfromnum(int num, int* ressize) {
    int sz = ((int) floor(log10(num)) + 1);
    wchar_t* text = ecalloc(sz + 1, sizeof(wchar_t));

    int prevpower = 1;
    for (int i = 0; i < sz; i++) {
        int digit = ((num % (int)pow(10, i + 1)) - (num%prevpower))/(prevpower);
        text[sz - i - 1] = int_to_wstr_char_array[digit];
        prevpower = (int) pow(10, i + 1);
    }

    *ressize = sz + 1;

    return text;
}

bool wstrcmp(const wchar_t* first, const wchar_t* second, int szfirst, int szsecond) {
    if (szfirst == szsecond) {
        for (int i = 0; i < szfirst; i++) {
            if (first[i] != second[i]) return false;
        }

        return true;
    }

    return false;
}

u8* wstrdgr(const wchar_t* str, int sz) {
    const wchar_t* p = str;
    u8* cur = (u8*)p;
    u8* res = ecalloc(sz, sizeof(u8));
    for (int i = 0; i < sz; i++) res[i] = cur[2*i];
    return res;
}

u32 wstrlen(const wchar_t* str) {
    const wchar_t* t = str;
    while (*t != L'\0') t++;
    return t - str;
}

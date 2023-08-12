#ifndef WSTR_H_INCLUDED
#define WSTR_H_INCLUDED

#include <stdlib.h>
#include <stdbool.h>

#include "types.h"

typedef struct {
    wchar_t* str;
    u32 size;
} Wide_string;

wchar_t int_to_wstr_char_array[10];

void wstr_start();

wchar_t* wstrcat_in_tbuffer_render(const wchar_t* first, const wchar_t* second, int szfirst, int szsecond);
wchar_t* wstrcat(const wchar_t* first, const wchar_t* second, int szfirst, int szsecond, int* ressize); /* Concatenates 2 wchar_t strings given their sizes */
bool     wstrisnum(const wchar_t* str, int sz); /* Returns true if str represents a number */
u32      wstrtonum(const wchar_t* str, u32 sz); /* Turn a wide string into a number */
u32      wstrlen(const wchar_t* str); /* Returns the length of the given wstr without the terminator */
wchar_t* wstrfromnum(int num, int* ressize);
bool     wstrcmp(const wchar_t* first, const wchar_t* second, int szfirst, int szsecond);
u8*      wstrdgr(const wchar_t* str, int sz); /* Turns a wstr (that only consists of ASCII chars) into a normal string. String char size is the same */

#define GSTRINIT(string_constant, string_itself) string_constant.str = string_itself; string_constant.size = wstrlen(string_constant.str) + 1

Wide_string STR_COMMSG_INVALID;
Wide_string STR_COMMSG_NEED_EXACTARGS;
Wide_string STR_COMMSG_INVALID_ARGTYPE;
Wide_string STR_COMMSG_INVALID_ARGRANGE;

#endif

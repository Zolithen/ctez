#ifndef WSTR_H_INCLUDED
#define WSTR_H_INCLUDED

#include <stdlib.h>
#include <stdbool.h>

#include "types.h"

typedef struct {
    wchar_t* str;
    u32 size;
} Wide_string;

typedef struct {
    u8* str;
    u32 size;
} Narrow_string;

wchar_t int_to_wstr_char_array[10];

void wstr_start();

// Wide string functions
u8*      wstrdgr(const wchar_t* str, u32  sz); /* Turns a wstr (that only consists of ASCII chars) into a normal string. String char size is the same */
wchar_t* wstrcat_in_tbuffer_render(const wchar_t* first, const wchar_t* second, int szfirst, int szsecond);
wchar_t* wstrcat(const wchar_t* first, const wchar_t* second, u32 szfirst, u32 szsecond, u32* ressize); /* Concatenates 2 wchar_t strings given their sizes */
wchar_t* wstrcat_second_no_terminator(const wchar_t* first, const wchar_t* second, u32 szfirst, u32 szsecond, u32* ressize);
wchar_t* wstrfromnum(u32 num, u32* ressize); /* Creates an string from a number */
wchar_t* wstrfilefrompath(const wchar_t* str, u32 sz, u32* ressize); /* Get the file name out of a path */
bool     wstrcmp(const wchar_t* first, const wchar_t* second, u32 szfirst, u32 szsecond); /* Returns if both strings are the same */
bool     wstrisnum(const wchar_t* str, u32 sz); /* Returns true if str represents a number */
u32      wstrtonum(const wchar_t* str, u32 sz); /* Turn a wide string into a number */
u32      wstrlen(const wchar_t* str); /* Returns the length of the given wstr without the terminator */

// Narrow string functions
u8* nstrfilefrompath(const u8* str, u32 sz, u32* ressize);
u32 nstrlen(const u8* str);
u8* nstrcat(const u8* first, const u8* second, u32 szfirst, u32 szsecond, u32* ressize);

#define GSTRINIT(string_constant, string_itself) string_constant.str = string_itself; string_constant.size = wstrlen(string_constant.str) + 1
#define GNSTRINIT(string_constant, string_itself) string_constant.str = string_itself; string_constant.size = nstrlen(string_constant.str) + 1

Narrow_string STR_TEMPORAL_FILE_EXTENSION;

Wide_string STR_SYS_NEW_BUFFER;
Wide_string STR_BUFFER_NOT_LINKED_TO_FILE;

Wide_string STR_COMMSG_INVALID_ARGTYPE;
Wide_string STR_COMMSG_INVALID_ARGRANGE;
Wide_string STR_COMMSG_INVALID_COMMAND;
Wide_string STR_COMMSG_INVALID_PATH;
Wide_string STR_COMMSG_NEED_EXACTARGS;
Wide_string STR_COMMSG_BUFFER_ALREADY_BOUND;

Wide_string STR_COMMSG_SAVED_FILE;

#endif

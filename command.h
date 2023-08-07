#ifndef COMMAND_H_INCLUDED
#define COMMAND_H_INCLUDED

#include <stdlib.h>
#include "strlist.h"

typedef enum {
    COMRESP_OK = 1,
    COMRESP_NOTFOUND = 2
} Command_response_code;

typedef struct {
    Command_response_code resp;
    wchar_t* msg;
    int msgl;
} Command_response;

Wide_string_list* command_parse(wchar_t* com, int coml);
Command_response command_execute(Wide_string_list* com);

#endif // COMMAND_H_INCLUDED

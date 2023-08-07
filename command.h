#ifndef COMMAND_H_INCLUDED
#define COMMAND_H_INCLUDED

#include <stdlib.h>
#include "strlist.h"

typedef enum {
    COMRESP_OK = 1,
    COMRESP_NOGIVEN = 2, // No command given
    COMRESP_INVALID = 3, // Command not found
    COMRESP_NEEDARGS = 4, // Need more arguments
    COMRESP_NOTFOUND = 5 // File/directory not found
} Command_response_code;

typedef struct {
    Command_response_code resp;
    Wide_string msg;
} Command_response;

Wide_string_list* command_parse(wchar_t* com, int coml);
Command_response command_execute(Wide_string_list* com);

#endif // COMMAND_H_INCLUDED

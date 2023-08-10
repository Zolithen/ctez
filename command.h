#ifndef COMMAND_H_INCLUDED
#define COMMAND_H_INCLUDED

#include <stdlib.h>
#include "strlist.h"

typedef enum {
    COMRESP_OK = 1,
    COMRESP_INVALID = 2, // Command not found
    COMRESP_NEEDARGS = 3, // Need more arguments
    COMRESP_NOTFOUND = 4 // File/directory not found
} Command_response_code;

typedef struct {
    Command_response_code resp;
    Wide_string msg;
} Command_response;

Wide_string_list* command_parse(wchar_t* com, int coml);
Command_response command_execute(Wide_string_list* com);

// Default command messages
void command_msg_setup_defaults();

Wide_string COMMAND_MSG_INVALID;
Wide_string COMMAND_MSG_NEEDARGS;

#endif // COMMAND_H_INCLUDED

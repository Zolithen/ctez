#ifndef COMMAND_H_INCLUDED
#define COMMAND_H_INCLUDED

#include <stdlib.h>
#include "strlist.h"

typedef enum {
    COMRESP_OK = 1,
    COMRESP_INVALID = 2, // Command not found
    COMRESP_NEEDARGS = 3, // Need more arguments
    COMRESP_NOTFOUND = 4 // File/directory not found
} Command_response_code; // Is it really needed?

typedef struct {
    Command_response_code resp;
    Wide_string msg;
} Command_response;

Wide_string_list* command_parse(wchar_t* com, int coml);
Command_response command_execute(Wide_string_list* com);

// Command messages
void command_msg_setup_defaults();
void comerror_file_not_found(Wide_string* file_name);
void comerror_file_invalid();

Wide_string COMMAND_MSG_INVALID;
Wide_string COMMAND_MSG_NEEDARGS;
Wide_string COMMAND_MSG_INVALID_ARGTYPE;

#endif // COMMAND_H_INCLUDED

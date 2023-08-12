#ifndef COMMAND_H_INCLUDED
#define COMMAND_H_INCLUDED

#include <stdlib.h>
#include <stdbool.h>

#include "types.h"
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

typedef enum {
    COMARGTYPE_ANY,
    COMARGTYPE_INT
} Command_argument_type;

Wide_string_list* command_parse(wchar_t* com, int coml);
void command_execute(Wide_string_list* com);

// Utilities for command error reporting
bool comerror_argtype_expect_exactly(Wide_string_list* com, u32 argnum, Command_argument_type argtype);
bool comerror_argrange_expect_exactly(Wide_string_list* com, u32 argnum, int minimum, int maximum); /* We expect the argument to be already an integer */

// Command messages
bool comerror_args_expect_exactly(Wide_string_list* l, int numargs);
void comerror_file_not_found(Wide_string* file_name);
void comerror_file_invalid();

#endif // COMMAND_H_INCLUDED

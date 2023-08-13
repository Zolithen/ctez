#ifndef COMMAND_H_INCLUDED
#define COMMAND_H_INCLUDED

#include <stdlib.h>
#include <stdbool.h>

#include "types.h"
#include "strlist.h"
#include "buffer.h"

typedef struct {
    Wide_string name;
    void (*func)(Wide_string_list* com);
} Command;

typedef enum {
    COMARGTYPE_ANY,
    COMARGTYPE_INT
} Command_argument_type;

Wide_string_list* command_parse(wchar_t* com, int coml);
void command_execute(Wide_string_list* com);

// Utilities for command error reporting
/* Checks if there's exactly numargs arguments */
bool comexpect_args(Wide_string_list* com, int numargs);
/* Checks if the given argument is of type argtype */
bool comexpect_argtype(Wide_string_list* com, u32 argnum, Command_argument_type argtype);
/* We expect the argument to be already an integer . Checks if the number the argument provides is inside the range (both min & max inclusive)*/
bool comexpect_argrange(Wide_string_list* com, u32 argnum, int minimum, int maximum);
/* We expect the argument to be already an integer. Checks if the number the argument provides is false in the given array */
bool comexpect_false_in_array(Wide_string_list* com, u32 argnum, bool* arr, u32 arrsize);
bool comexpect_true_in_array(Wide_string_list* com, u32 argnum, bool* arr, u32 arrsize);

/* Checks if bufid is a buffer non bound to some window */
bool comexpect_buffer_unbound(TBUFID bufid);

// Command messages
void comerror_file_not_found(Wide_string* file_name);
void comerror_file_invalid();

#endif // COMMAND_H_INCLUDED

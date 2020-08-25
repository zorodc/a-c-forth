#ifndef EVAL_H
#define EVAL_H

#include "ForthTypes.h"

enum object_type { O_EOF, O_ERROR, O_WORD, O_INTEGRAL, O_STRING};
enum  error_type{ E_BADNUM, E_NOTINDICT, E_LINETOOLONG, E_UNTERMINATED_STRING };
struct error {
	const char* bad_string; // bad_string can be NULL if none is applicable.
	enum error_type   type;
};
typedef union {
	struct error error;
	char*         word;
	char*       string;
	double  fractional;
	long      integral;
} Object;

_Bool Eval(struct State state, enum object_type(*getobj)(Object*),
           // handleError can be NULL
           void(*handleError)(struct error));

#endif // EVAL_H

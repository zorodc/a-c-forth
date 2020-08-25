#ifndef FORTH_TYPES_H
#define FORTH_TYPES_H
#include "Dict.h"
#include "Stack.h"

/* Struct for global state, passed around. Required by forth semantics. */
struct State {
	Dict namespace;
	Stack stack;

	/// Stores information about the types of the things that are one the stack.
	Stack types; /* Can be NULL if typing is not intended. */
};

// TODO: Have a function type that stores a forth 'expression.'
enum function_type { F_BUILTIN };
typedef struct {
	union {
		void(*builtin)(struct State state);
	}data; // C99 compat
	enum function_type type;
}ForthWord;

enum datum_type { T_INT, T_STRING };
typedef union {
	long  Int;
	char* String;
}ForthDatum;

#endif // FORTH_TYPES_H

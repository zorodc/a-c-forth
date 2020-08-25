#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include "Dict.h"
#include "Stack.h"
#include "ForthTypes.h"
#include "Eval.h"
#include "Debug.h"

/* Returns false on encountering the end of the file. */
_Bool Eval(struct State state, enum object_type(*getobj)(Object*),
           // handleError can be NULL
           void(*handleError)(struct error)) {
	assert(state.namespace);
	assert(state.stack);
	// state.types can be NULL a user is attempting to forego type checking
	// in order to improve speed. All other members of (struct State) cannot.
	assert(getobj);

#define TYPING_ON (1 << 8) /* Set the 8th bit. */
#define TYPING_OFF (0)
	const unsigned TypeState = (state.types) ? TYPING_ON : TYPING_OFF;
	DEBUG_DO({
		if (state.types)
			DEBUG_PRINT("Eval called with typing ON.\n");
		else
			DEBUG_PRINT("Eval called with typing OFF.\n");
		});

	/// Fairly ugly switch statement.
	Object o;
	switch (getobj(&o)|TypeState) {
	case O_WORD|TYPING_ON: // fallthrough
	case O_WORD: {
		DEBUG_PRINTF("Eval: Got an O_WORD from getobj: `%s`.\n", o.word);
		ForthWord fw;
		/* Try lookup: if lookup fails, break. */
		/* TODO: Handle bad lookups: they mean an undefined symbol is used. */
		if (!DictGet(state.namespace, &o.word, &fw)) {
			if(handleError) handleError((struct error){.type=E_NOTINDICT,
			                                           .bad_string=o.word});
			goto freeWord; }
		switch(fw.type) {
		case F_BUILTIN:
			DEBUG_PRINTF("Eval: Lookup with `%s` provided 0x%llx.\n",
			              o.word, (long long) fw.data.builtin);
			/* Call builtin function, allowing it to mutate state. */
			fw.data.builtin(state);
			break;
			/* TODO: add and manage additional forth word types. */
		default:
			fprintf(stderr,
			        "Bad value %d found for type in dict, "
			        "for lookup string `%s`.",
			        fw.type, o.word);
			break; }
		/* TODO: Allocate words in a static buffer, if sensible. */
		freeWord:
		free(o.word);
	} break;
	case O_INTEGRAL|TYPING_ON: {
		enum datum_type t = T_INT;
		StackPush(state.types, &t); }
		// fallthrough
	case O_INTEGRAL: {
		DEBUG_PRINTF("Eval: Got an O_INTEGRAL: `%ld`.\n", o.integral);
		ForthDatum fd;
		fd.Int = o.integral;
		StackPush(state.stack, &fd);
	} break;
	case O_STRING|TYPING_ON: {
		enum datum_type t = T_STRING;
		StackPush(state.types, &t); }
		// fallthrough
	case O_STRING: {
		DEBUG_PRINTF("Eval: Got an O_STRING: `%s`.\n", o.string);
		ForthDatum fd;
		fd.String = o.string;
		StackPush(state.stack, &fd);
	} break;
	case O_ERROR|TYPING_ON: // fallthrough
	case O_ERROR:
		if (handleError) handleError(o.error);
		break;
	case O_EOF|TYPING_ON: // fallthrough
	case O_EOF:
		return false;
		break;
	default:
		fprintf(stderr,
		        "BUG: Bad value passed to Eval from "
		        "function pointer getobj.\n");
		break; }
	return true;
}

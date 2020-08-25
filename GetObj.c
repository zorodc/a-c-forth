#include <stdio.h>
#include <stdlib.h> // alloca
#include <alloca.h>
#include <assert.h>
#include <stdbool.h>
#include <ctype.h>

#include "GetObj.h"
#include "Eval.h"
#include "ForthTypes.h"

#include "Debug.h"
#include "Strdup.h"

/* Maximum amount of characters grabbed at one time. */
#define MAX_GRAB_SIZE 512

#define ISSEP(c) ((c) == ' ' || (c) == '\t' || (c) == '\n')

/* Global stream */
static FILE* G_Stream = NULL;

static inline unsigned long readIntegral(const char* ringSub,
                                         enum object_type* typeSlot,
                                         Object* objectSlot) {
	assert(ringSub);
	assert(typeSlot);
	assert(objectSlot);

	char* endptr;
	long l = strtol(ringSub, &endptr, 0);

	/* If a bad character was encountered, and it isn't a separator. */
	if (endptr[0] && !ISSEP(endptr[0])) {
		*typeSlot = O_ERROR;
		objectSlot->error = (struct error) {.type = E_BADNUM,
		                                    .bad_string = ringSub};
		goto readIntegral_Return; }

	*typeSlot = O_INTEGRAL;
	objectSlot->integral = l;

	DEBUG_PRINTF("readIntegral: Read %lu\n", (unsigned long) (endptr-ringSub));
	readIntegral_Return:

	 // Skip to the end of string or separator,
	 // so as to not consume erroneous chars still part of the integral literal.
	while (*endptr && !ISSEP(*endptr)) ++endptr;
	return (unsigned long) (endptr-ringSub);
}

static inline unsigned long readString(const char* ringSub,
                                       enum object_type* typeSlot,
                                       Object* objectSlot) {
	assert(ringSub);
	assert(typeSlot);
	assert(objectSlot);

	++ringSub; // Skip past initial '"'
	unsigned long i;
	for(i = 0; ringSub[i] != '"'; ++i)
		if (!ringSub[i]) {
			*typeSlot = O_ERROR;
			objectSlot->error = (struct error) {.type = E_UNTERMINATED_STRING,
			                                    .bad_string = ringSub};
			goto readString_Return; }
	*typeSlot = O_STRING;
	objectSlot->string = pstrndup(ringSub, i);

	DEBUG_PRINTF("readString: Read %lu\n", i+2);
readString_Return:
	return i+2; /* 2, one for each '"'. */
}

static inline unsigned long readWord(const char* ringSub,
                                     enum object_type* typeSlot,
                                     Object* objectSlot) {
	assert(ringSub);
	assert(typeSlot);
	assert(objectSlot);

	/* TODO: Handle errors. */
	unsigned long i;
	for(i = 0; !ISSEP(ringSub[i]); ++i);
	*typeSlot = O_WORD;
	objectSlot->word = pstrndup(ringSub, i);
	return i;
}

static int fpeek(FILE* fd) {
	int c = fgetc(fd);
	ungetc(c, fd);
	return c;
}

/* Architecturally oversimplified tokenizer + parser combo. */
/* Some languages are so simple it becomes reasonable
   to implement a lexer and parser as a single entity, rather
   than having to generate lexemes and pass over them seperately.
   Forth is one such language. */
/* Gets a single lexeme-like entity. */
static enum object_type GetObj_(Object* slot) {
	static char line[MAX_GRAB_SIZE] = {'\0'};
	static unsigned long idx = 0;
	unsigned long readLength = 0;
	enum object_type ret;

	goto start;
Refill:
	// TODO: implement Refill: properly.
	// TODO: Use fgets only if interraction is required, fread otherwise.
	// TODO: Allow the interpretation of a string, independent of its source.
	if (fpeek(G_Stream) == EOF) return O_EOF;
	fgets(line, sizeof(line)/sizeof(*line), G_Stream);
	idx = 0;
start:
	for(;;) {
		DEBUG_PRINTF("Switching on %d, `%c`\n", line[idx], line[idx]);
		DEBUG_PRINTF("idx == %lu\n", idx);
		switch(line[idx]) {
		case '\0':
			goto Refill;
			break;
		/***** WHITESPACE *****/
		case '\n':
		case ' ':
		case '\t':
			++idx;
			break;

		/***** LITERALS *****/
		case '-':
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			/* Parse number. */
			readLength = readIntegral(line + idx, &ret, slot);
			idx += readLength;
			return ret;
			break;

		case '"':
			/* Parse string. */
			readLength = readString(line + idx, &ret, slot);
			idx += readLength;
			return ret;
			break;

		/***** WORD DEFINITIONS *****/

		case ':':
			break;

		case ';':
			break;

		/***** WORDS *****/
		default:
			readLength = readWord(line + idx, &ret, slot);
			idx += readLength;
			return ret;
			break; }}
}

static enum object_type GetObj_OLD_(Object* slot) {
	assert(slot);

	/* Store strings as they are constructed. */
	Stack string_vect = alloca(StackSize());
	string_vect = StackNew(string_vect, sizeof(char), NULL);
	int c;

	struct {
		_Bool seen_nonwhite;
		signed int vect_contents; // enum object_type, w/ -1
	}state = {false, -1};

	for(;;) {
	switch (c=fgetc(G_Stream)) {
	/* 0-9: Number */
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		state.seen_nonwhite = true;
		state.vect_contents = O_INTEGRAL;
		StackPush(string_vect, &c);
		break;
	case '"':
		state.seen_nonwhite = true;
		if (O_STRING == state.vect_contents)
			goto Return;
		else state.vect_contents = O_STRING;
		break;
	/* Whitespce: End of token. */
	case ' ': // fallthrough
	case '\t':
		if (O_STRING == state.vect_contents) {
			StackPush(string_vect, &c);
			break; }
		// fallthrough
	case '\n':
		/* Skip whitespace until a token is seen. */
		if(!state.seen_nonwhite)
			break;
		else /* Return interpretation of current token. */
			goto Return;
		break;
	case EOF:
		if (state.vect_contents == -1)
			state.vect_contents = O_EOF;
		goto Return;
		break;
	default:
		state.seen_nonwhite = true;
		if (state.vect_contents != O_STRING)
			state.vect_contents = O_WORD;
		StackPush(string_vect, &c);
		break;
	}}

/* Token successfully parsed. Return it. */
Return:
	c = '\0';
	StackPush(string_vect, &c);
	switch (state.vect_contents) {
	case O_INTEGRAL:
		DEBUG_PRINT("GetObj_: Returning O_INTEGRAL.\n");
		slot->integral = atoi(StackPeek(string_vect));
		break;
	case O_STRING:
		DEBUG_PRINT("GetObj_: Returning O_STRING.\n");
		slot->string = pstrdup(StackPeek(string_vect));
		break;
	case O_WORD:
		DEBUG_PRINT("GetObj_: Returning O_WORD.\n");
		slot->word = pstrdup(StackPeek(string_vect));
		break;
	case O_EOF:
		break;
	case O_ERROR:
		fprintf(stderr, "O_ERROR: UNIMPLEMENTED\n");
		/* TODO: Implement O_ERROR in GetObj */
		break;
	default:
		fprintf(stderr,
		        "BUG: Invalid value %d stored in vect_contents.\n",
		        state.vect_contents);
		break;
	}
	StackDelete(string_vect);
	return state.vect_contents;
}

/* Takes a FILE* and returns a function pointer to a GetObj. */
/* Only one instance can be used at once, because there are no closures in C. */
/* You could use a macro to implement them, however. */
GetObjFN CreateGetObj(FILE* stream) {
	assert(stream);

	G_Stream = stream;
	return GetObj_;
}

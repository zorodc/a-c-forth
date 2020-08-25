#include <stdio.h>
#include <stdlib.h> // free
#include <string.h> // strdup, if available, and strlen & strcpy if not.
#include <stdbool.h>

#include "Dict.h"
#include "Stack.h"
#include "Assert.h"
#include "ForthTypes.h"
#include "Builtins.h"

#include "Strdup.h"

/* Private state initialized by ImportBuiltins. */
static _Bool (*Pop )(struct State s, void* elemSpace) = NULL;
static _Bool (*Push)(struct State s, enum datum_type type,
                      const void* datum) = NULL;

/********** PRIVATE: UTILITY FUNCTIONS **********/
static _Bool PopWithTypeStack(struct State s, void* elemSpace)
{
	cassert(s.stack);
	cassert(s.types);

	enum datum_type _;

	if (!StackPop(s.types, &_)) /* Pop from TypeStack. */
		return false;
	return StackPop(s.stack, elemSpace);
}

static _Bool PopWithoutTypeStack(struct State s, void* elemSpace)
{
	cassert(s.stack);
	cassert(!s.types);

	return StackPop(s.stack, elemSpace);
}

static _Bool PushWithTypeStack(struct State s,
							   enum datum_type type, const void* datum)
{
	cassert(s.stack);
	cassert(s.types);

	if (!StackPush(s.types, &type))
		return false;

	return StackPush(s.stack, datum);
}

static _Bool PushWithoutTypeStack(struct State s,
								  enum datum_type _ __attribute__((unused)),
								  const void* datum)
{
	cassert(s.stack);
	cassert(!s.types);

	return StackPush(s.stack, datum);
}

/********** PRIVATE: BUILTIN FUNCTIONS **********/
static void HelloWorld(struct State _ __attribute__((unused)))
{
	puts("Hello, World!");
}

static void PopAndPrintIntegral(struct State s)
{
	ForthDatum d;
	if (Pop(s, &d))
		printf("%ld", d.Int);
	else {} // TODO: ERROR HANDLING
}

static void Newline(struct State _ __attribute__((unused)))
{
	putchar('\n');
}

static void Add(struct State s)
{
	ForthDatum d1;
	ForthDatum d2;

	if (!Pop(s, &d1)) goto Add_d1PopFail;
	if (!Pop(s, &d2)) goto Add_d2PopFail;
	d1.Int += d2.Int;
	if (!Push(s, T_INT, &d1.Int)) goto Add_SumPushFail;

	return;

	/* ERROR BLOCK */
Add_SumPushFail:
	; // ERROR HANDLING
Add_d2PopFail:
	;
Add_d1PopFail:
	;
}

static void Multiply(struct State s)
{
	ForthDatum d1;
	ForthDatum d2;

	if (!Pop(s, &d1)) goto Multiply_d1PopFail;
	if (!Pop(s, &d2)) goto Multiply_d2PopFail;
	d1.Int *= d2.Int;
	if (!Push(s, T_INT, &d1.Int)) goto Multiply_ProductPushFail;

Multiply_ProductPushFail:
	;
Multiply_d2PopFail:
	;
Multiply_d1PopFail:
	;
}

static void PrintLn(struct State s)
{
	ForthDatum d;
	if (Pop(s, &d)) {
		puts(d.String);
		free(d.String);
	} else {} // ERROR HANDLING
}

static void Print(struct State s)
{
	ForthDatum d;
	if (Pop(s, &d)) {
		fputs(d.String, stdout);
		free(d.String);
	} else {} // ERROR HANDLING
}

/********** PUBLIC **********/
_Bool ImportBuiltins(Dict namespace_to_mutate, _Bool typing_onp) {
	ForthWord fw; fw.type = F_BUILTIN;
	char* s;

	if (typing_onp) {
		Push = PushWithTypeStack;
		Pop  = PopWithTypeStack;
	} else {
		Push = PushWithoutTypeStack;
		Pop  = PopWithoutTypeStack;
	}

	/* Add items to the function namespace. */
	s = pstrdup("HelloWorld");
	fw.data.builtin = HelloWorld;
	if(!DictAdd(namespace_to_mutate, &s, &fw)) return false;

	s = pstrdup(".");
	fw.data.builtin = PopAndPrintIntegral;
	if(!DictAdd(namespace_to_mutate, &s, &fw)) return false;

	s = pstrdup("+");
	fw.data.builtin = Add;
	if(!DictAdd(namespace_to_mutate, &s, &fw)) return false;

	s = pstrdup("*");
	fw.data.builtin = Multiply;
	if(!DictAdd(namespace_to_mutate, &s, &fw)) return false;

	s = pstrdup("nl");
	fw.data.builtin = Newline;
	if(!DictAdd(namespace_to_mutate, &s, &fw)) return false;

	s = pstrdup("PrintLn");
	fw.data.builtin = PrintLn;
	if(!DictAdd(namespace_to_mutate, &s, &fw)) return false;

	s = pstrdup("Print");
	fw.data.builtin = Print;
	if(!DictAdd(namespace_to_mutate, &s, &fw)) return false;

	return true;
}

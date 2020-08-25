/*
 * The following is an old forth interpreter I wrote while still in high school.
 *
 * It has a few builtin functions. Look in the 'Builtins.c' file.
 * A small demo program you can feed to its STDIN is in 'MyProgram.forth.'
 * It does not currently have the abiltiy to humor user-defined functions.
 *
 * It has a somewhat novel hash table design, using a bitset to store metadata.
 * Further, one can swap the 'Dict.h' symlink for one to 'AssocDictAsDict.h'
 *  to change the program's use of the normal hash table, to a linear search.
 *
 * I've done my best to fix some of the inconsistent comment conventions,
 *  as well as the intentation (I wrote this before I had whitespace-mode!),
 *  and it's mostly caught up. But, if it's odd in a few places, this'd be why.
 *
 * Also - there may be a few lingering bugs in the HT implementation.
 * It works often enough that none of my trivial programs fail, though.
 */

/* Forth interpreter */
/* Copyright (c) Daniel C., 2017 */

/* Fun Forth info:
   http://www.angelfire.com/in/zydenbos/WhatisForth.html */

#include <stdio.h>
#include <stdlib.h> // free
#include <string.h> // strcmp
#include "Dict.h"
#include "Stack.h"
#include "ForthTypes.h"

#include "Alloca.h"
#include "Builtins.h"
#include "Eval.h"
#include "GetObj.h"
#include "CleanLeaks.h"

void cstrcfree(void* v);
void ErrorHandler(struct error e);

void cstrcfree(void* v)
{
	free(*(char**)v);
}

void ErrorHandler(struct error e)
{
	switch(e.type) {
	case E_UNTERMINATED_STRING:
		fprintf(stderr, "SYNTAX ERROR: Unterminated string `%s`.\n",
		        e.bad_string);
		break;
	case E_BADNUM:
		fprintf(stderr, "SYNTAX ERROR: Bad numeric literal `%s`.\n",
		        e.bad_string);
		break;
	case E_NOTINDICT:
		if (e.bad_string)
			fprintf(stderr, "ERROR: Failed lookup on `%s`.\n", e.bad_string);
		else fprintf(stderr, "ERROR: Failed lookup.\n");
		return;
		break;
	case E_LINETOOLONG:
		if (e.bad_string)
			fprintf(stderr, "ERROR: Token `%s` too long.\n", e.bad_string);
		else fprintf(stderr, "ERROR: Token too long.\n");
		break;
	default:
		fprintf(stderr,
		        "Unhandled enum error_type instance or invalid value passed to "
		        "ErrorHandler callback ostensibly by Eval: %d.\n", e.type);
		break; }
}

/// Global state; Passed by reference to mutators explicitly.
static Dict  G_NameSpace;
static Stack G_ForthStack;
static Stack G_TypeStack;

#include "Strdup.h"
#include <assert.h>

void test(void) {
		HashDict hd; PALLOCA(hd, HashDictSize());
	hd = HashDictNew(hd, 0, sizeof(char*), sizeof(char*),
	                 cstrcSimpleHash, cstrcEq, cstrcfree, cstrcfree);
	if (!hd) puts("HashDict initialization failed.\n");

	char* key;
	char* val;
	char* valspc;

	const char* keyarr[] = {"Bobby", "Billy", "Jingo", "Jango", "Jingaling",
	                        "Meme lover", "Mr. Mays", "Senator Standalone", "Bobby Fischer",
	                        "Barack Obama", "Ronald Raygun", "JFK", "Moon-creature",
	                        "Hey there", "Who you gonna call?", "Grabingo"};
	const char* valarr[] = {"Bob", "Bill", "Jinga-ling", "Jan", "Jinga",
	                        "Cool dude", "Deals", "Fellow", "Author",
	                        "President", "Pew-Pew", "Nice hair", "Lunar",
	                        "Hi!", "Ghostbusters?", "Ok"};
	assert(sizeof(keyarr)/sizeof(*keyarr) == sizeof(valarr)/sizeof(*valarr));
	for (size_t i = 0;
	i < sizeof(valarr)/sizeof(*valarr) &&
	i < sizeof(keyarr)/sizeof(*keyarr);
	++i) {
	key = pstrdup(keyarr[i]);
	val = pstrdup(valarr[i]);
	if (!HashDictAdd(hd, &key, &val)) {
	assert(0);
	free(key); free(val); }
	assert(HashDictHas(hd, &key));
	valspc = NULL;
	HashDictGet(hd, &key, &valspc);
	printf("Got %s.\n", valspc);
	}
	for (size_t i = 0;
	i < sizeof(valarr)/sizeof(*valarr) &&
	i < sizeof(keyarr)/sizeof(*keyarr);
	++i) {
	const char* key = keyarr[i];
	const char* val = valarr[i];
	assert(HashDictHas(hd, &key));
	valspc = NULL;
	HashDictGet(hd, &key, &valspc);
	printf("For %s got %s, should get %s.\n", key, valspc, val);
	}

	HashDictDelete(hd);
}

int main(void)
{
	/* BEGIN TEST */
	#ifdef TEST
	test();
	#endif /* TEST */

	/// Initialize the Namespace that is to contain defined functions.
	PALLOCA(G_NameSpace, DictSize());
	G_NameSpace = DictNew(G_NameSpace, 0, sizeof(char*), sizeof(ForthWord),
	                      cstrcSimpleHash, cstrcEq, cstrcfree, NULL);
	if (!G_NameSpace) {
		fprintf(stderr, "FAIL: "
		        "NameSpace object could not be successfully initialized.\n");
		return 1;}
	else puts("OK: NameSpace object successfully initialized.");

	/// Initialize the global stack that is to be manipulated by builtins.
	PALLOCA(G_ForthStack, StackSize());
	G_ForthStack = StackNew(G_ForthStack, sizeof(ForthDatum), NULL);
	if (!G_ForthStack) {
		fprintf(stderr,
		        "FAIL: Global stack could not be successfully initialized.\n");
		return 1;}
	else puts("OK: Global stack successfully initialized.");

	/// Initialize global typestack.
	PALLOCA(G_TypeStack, StackSize());
	G_TypeStack = StackNew(G_TypeStack, sizeof(enum datum_type), NULL);
	if (!G_TypeStack) {
		fprintf(stderr,
		        "FAIL: Type tracker could not be successfully initialized.\n");
		return 1;}
		else puts("OK: Type tracker successfully initialized.");

	/// Import builtins into namespace.
	if(!ImportBuiltins(G_NameSpace, (_Bool)G_TypeStack)) {
		fprintf(stderr,
		        "FAIL: Builtin functions could not be imported.\n"
		        "Likely cause: lack of memory.\n");
		return 1; }
	else puts("OK: Builtin functions imported to namespace successfully.");

	/// Successful initialization!
	puts("Thus Spake the Interpreter, 'Go FORTH and love the stack.'");

	/// Main loop
	while(Eval((struct State){G_NameSpace, G_ForthStack, G_TypeStack},
			   CreateGetObj(stdin), ErrorHandler));

	/// Clean up.
	//  Free leftover items on the global stack.
	if (G_TypeStack)
		CleanLeaks(G_ForthStack, G_TypeStack, stderr);

	if (G_TypeStack) StackDelete(G_TypeStack);
	StackDelete(G_ForthStack);
	DictDelete(G_NameSpace);
	return 0;
}

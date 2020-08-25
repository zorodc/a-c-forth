#include <stdlib.h> // malloc
#include <string.h> // memcpy
#include <assert.h>
#include <stddef.h> // size_t
#include <stdbool.h>
#include "Stack.h"

struct Stack {
	char* stackBuf;

	void(*elemfree)(void*);

	size_t elemsize;
	size_t allocated;
	size_t n;
};

#define STACK_DEFAULT_SIZE 16

size_t StackSize(void)
{
	return sizeof(struct Stack);
}

Stack StackNew(void* memory, size_t elemsize,
               // elemfree can be NULL.
               void(*elemfree)(void*))
{
	assert(memory);   // STRICT
	assert(elemsize); // STRICT
	if (!memory) return NULL;

	Stack s = memory;
	s->stackBuf = malloc(STACK_DEFAULT_SIZE * elemsize);
	assert(s->stackBuf); // STRICT
	if (!s->stackBuf) return NULL; // Return NULL on allocation failure.

	s->elemfree = elemfree;
	s->elemsize = elemsize;

	s->allocated = STACK_DEFAULT_SIZE;
	s->n = 0;

	return s;
}

_Bool StackIsEmpty(Stack s)
{ return 0 == s->n; }

_Bool StackPush(Stack s, const void* elem)
{
	assert(s); // STRICT
	assert(elem);
	if (!s) return false;

	assert(s->n <= s->allocated); // STRICT
	if (s->allocated >= s->n) {
		char* ptr = realloc(s->stackBuf, s->allocated * s->elemsize * 2);
		if (!ptr) // Failed allocation, leave s's contents untouched.
			return false;
		s->stackBuf = ptr;
		s->allocated *= 2; }

	// Copy element into free slot.
	memcpy(s->stackBuf + s->n * s->elemsize, elem, s->elemsize);
	++s->n;
	return true;
}

_Bool StackPop(Stack s,
               // elemSpace can be NULL.
               void* elemSpace)
{
	assert(s); // STRICT
	if (!s) return false;

	if (!s->n) return false;
	--s->n;
	if (elemSpace)
		memcpy(elemSpace, s->stackBuf + s->n * s->elemsize, s->elemsize);
	return true;
}

void* StackPeek(const Stack s)
{
	return s->stackBuf;
}

void StackDelete(Stack s)
{
	assert(s); // STRICT
	if (!s) return;


	if (!s->elemfree) goto sd_free;
	for (unsigned i = 0; i < s->n; ++i) {
		char* curr_elem = s->stackBuf + i * s->elemsize;
		s->elemfree(curr_elem); }
sd_free:
	free(s->stackBuf);
}

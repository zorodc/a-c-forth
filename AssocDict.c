#include <stdlib.h> // malloc
#include <string.h> // memcpy
#include <assert.h>
#include <stddef.h> // size_t
#include <stdbool.h>
#include "AssocDict.h"

/********** Simple associative array with linear search lookups. **********/
/* This is, of course, a placeholder for a more efficient (but more complex)
   data structure, like a hash table,
   which isn't really that complex, but can be a bit harder to debug.
   Hash tables have some edge cases that I don't want to waste time
   testing for, until I get this damn thing working. */

#define ASSOCDICT_DEFAULT_PAIRS 16

struct AssocDict {
	char* dictBuf;

	_Bool(*keyEq)(const void*, const void*);
	void(*keyfree)(void*);
	void(*valfree)(void*);

	size_t keysize;
	size_t valsize;

	size_t allocated;
	size_t n;
};

size_t AssocDictSize(void)
{
	return sizeof(struct AssocDict);
}

AssocDict AssocDictNew(void* memory, size_t keysize, size_t valsize,
			 _Bool(*keyEq)(const void*, const void*),
			 // [key,val]free can be NULL.
			 void(*keyfree)(void*), void(*valfree)(void*))
{
	assert(memory);  // STRICT
	assert(keysize); // STRICT
	assert(valsize); // STRICT
	assert(keyEq);
	if (!memory) return memory;

	AssocDict dict = memory;
	dict->dictBuf = malloc(ASSOCDICT_DEFAULT_PAIRS * (keysize + valsize));
	assert(dict->dictBuf); // STRICT
	if (!dict->dictBuf) return NULL; // Return NULL on allocation failure.

	dict->keyfree = keyfree;
	dict->valfree = valfree;
	dict->keysize = keysize;
	dict->valsize = valsize;

	dict->keyEq = keyEq;
	dict->allocated = ASSOCDICT_DEFAULT_PAIRS;
	dict->n = 0;

	return dict;
}

_Bool AssocDictAdd(struct AssocDict* d, const void* key, const void* val)
{
	assert(d); // STRICT
	assert(key);
	assert(val);

	if (!d) return false;

	assert(d->n <= d->allocated); // STRICT
	if (d->n >= d->allocated) {
		char* ptr = realloc(d->dictBuf,
							d->allocated * (d->valsize + d->keysize) * 2);
		if (!ptr) // Failed reallocation, leave d's contents untouched.
			return false;
		d->dictBuf = ptr;
		d->allocated *= 2; }

	// Copy key.
	memcpy(d->dictBuf + d->n * (d->keysize + d->valsize), key, d->keysize);
	// Copy value.
	memcpy(d->dictBuf + d->n * (d->keysize + d->valsize)
		   + d->keysize, val, d->valsize);

	++d->n;

	return true;
}

_Bool AssocDictHas(const struct AssocDict* d, const void* key)
{
	assert(d); // STRICT
	assert(key);
	if (!d) return false;

	return AssocDictGet(d, key, NULL);
}

_Bool AssocDictGet(const struct AssocDict* d, const void* key,
			  // valSpace can be NULL
			  void* valSpace)
{
	assert(d); // STRICT
	assert(key);
	if (!d) return false;

	for (unsigned long i = 0; i < d->n; ++i) {
		char* curr_key = d->dictBuf + i * (d->keysize + d->valsize);
		if (d->keyEq(curr_key, key)) {
			if (valSpace)
				memcpy(valSpace, curr_key + d->keysize, d->valsize);
			return true;
	}}
	return false;
}

void AssocDictDelete(AssocDict d)
{
	assert(d); // STRICT
	if (!d) return;

	if (!d->keyfree && !d->valfree) goto dd_free;
	for (unsigned i = 0; i < d->n; ++i) {
		char* curr_key = d->dictBuf + i * (d->keysize + d->valsize);
		if (d->keyfree) d->keyfree(curr_key);
		if (d->valfree) d->valfree(curr_key + d->valsize);
	}
dd_free:
	free(d->dictBuf);
}

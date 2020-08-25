#include <stdlib.h> // malloc, free
#include <string.h> // memcpy
#include <math.h>   // round
#include <assert.h>
#include <stdbool.h>
#include <stddef.h> // size_t

#include "HashDict.h"
#include "BitSet.h"
#include "Alloca.h"
#include "Assert.h"

#define DEFAULT_NSLOTS 16
/// The maximum number of consecutive rehashes allowed.
#define REHASH_LIMIT   8
/// round(<large-power-of-two> * factor) done twice ~= <large-power...> *2
/// The above effect ceases to be true for multiplications done after reaching 16,384, 2^14, such that (2^14)*2 != 2^15. With a DEFAULT_NSLOTS of 16, then the hash table can be rehashed to correct powers of two 10 times. Only on the  11th does it cease to produce a power of two, so one can expect 11 sane rehashes, because the twelth will be done by multiplying a large power of two by two.
/* TODO: Use a macro that grabs primes from a precomputed array. */
//#define RESIZE_FACTOR 1.4142
// (size_t) (round(*nSlots * RESIZE_FACTOR));
#define RESIZE_FACTOR 2

/* Data structure representation. */

// ~128 bytes in size, because of the composed BitSet member.
// The BitSet helps substantially in reducing the memory usage of the buf.
struct HashDict {
    /// Tracks which slots are in use and which are not.
    struct BitSet usedIndices;
    unsigned char* buf;

	_Bool (*keysEq) (const void* ep1, const void* ep2);
    size_t(*keyHash)(const void*);
    void  (*keyfree)(void*);
    void  (*valfree)(void*);

    size_t keysize;
    size_t valsize;

    /// Number of slots currently allocated.
    size_t nSlots;
};

/* TODO: Add appropriate casserts. */

/* Helper macros. */
#define VERIFY_CORRECT_BOUND(hd, idx)	              \
	( (hd)->nSlots <= BitSetBound(&(hd)->usedIndices) )
#define IN_RANGE(hd, idx)	                                              \
	( ((idx) < BitSetBound(&(hd)->usedIndices)) && ((idx) < (hd)->nSlots) )
/// Integral resize
#define RESIZE(old_size) ((old_size) * RESIZE_FACTOR)
#define KEYIDX(hd, idx) ( (hd)->buf + (idx)*((hd)->keysize+(hd)->valsize) )
#define VALIDX(hd, idx) ( KEYIDX((hd), (idx)) + (hd)->keysize )

/* Helper functions.
   - Helpers do not maintain class invariants, and therefore responsible use of them in the implementation of methods is expected.
   - Helpers with lowercase names impart no state changes. */

/// Returns the index, (aka 'slot number') of a particular key.
static inline size_t slotOf(const struct HashDict* hd, const void* key)
{ return hd->keyHash(key) % hd->nSlots; }
/* TODO: Add thread-safe optimization to slotOf, possibly by defining it
   as a macro that calls an implementation, slotOf_. */

/// Returns whether the slot `idx` of `hd` has a pair occupying it.
static inline _Bool occupied(const struct HashDict* hd, size_t idx)
{ iassert(IN_RANGE(hd, idx));
    return BitSetHas(&hd->usedIndices, idx); }

static inline _Bool Has(const struct HashDict* hd, const void* key, size_t idx)
{
	return occupied(hd, idx) && hd->keysEq(key, KEYIDX(hd, idx));
}

/// Returns whether the slot `idx` of `hd` has a pair occupying it.
static inline _Bool vacated(const struct HashDict* hd, size_t idx)
{ return !occupied(hd, idx); }

/// Blindly set the slot `idx` of `hd` as occupied.
static inline void Occupy(struct HashDict* hd, size_t idx) {
	iassert(IN_RANGE(hd, idx) && ":( extension not explicit");
	iassert(vacated(hd, idx) && ":( `idx` already occupied");
	BitSetAdd(&hd->usedIndices, idx); }

/// Blindly set the slot `idx` of `hd` as unoccupied.
static inline void Vacate(struct HashDict* hd, size_t idx) {
	iassert(IN_RANGE(hd, idx) && ":( `idx` larger than expected");
	iassert(occupied(hd, idx) && ":( `idx` already vacated");
	BitSetRemove(&hd->usedIndices, idx); }

/// Blindly put `key` and `val` at the requested `idx` in buf.
/// PREONDITION: hd[idx] is in range; hd[idx] is unoccupied.
/// POSTCONDITION: hd[idx] contains (key,val), but is not marked occupied.
/// NOTE: Leaves hd in invalid state; to rectify, mark `idx` as occupied.
static inline void PutAt(struct HashDict* hd,
                         const void* key, const void* val, size_t idx) {
	iassert(IN_RANGE(hd, idx));
	iassert(vacated(hd, idx));
	memcpy(KEYIDX(hd, idx), key, hd->keysize);
	memcpy(VALIDX(hd, idx), val, hd->valsize); }

/// Blindly destruct (key,val) at hd[idx] using hd->{key,val}free.
/// PRECONDITION: hd[idx] is in range; hd[idx] is occupied.
/// POSTCONDITION: hd[idx] is safely reusable, but not marked unoccupied.
/// NOTE: Places hd in invalid state; to rectify, mark `idx` as unoccupied.
static inline void DeleteAt(struct HashDict* hd, size_t idx) {
	iassert(IN_RANGE(hd, idx));
	iassert(occupied(hd, idx));
	if (hd->keyfree) hd->keyfree(KEYIDX(hd, idx));
	if (hd->valfree) hd->valfree(VALIDX(hd, idx));
}

/// Attempt to add an item without rehashing, returning true on success.
/// POSTCONDITION: (key,val) put in hd[idx], hd[idx] marked occupied.
/// NOTE: Invariants maintained.
static inline _Bool TryAdd(struct HashDict* hd,
                           const void* key, const void* val) {
	iassert(IN_RANGE(hd, slotOf(hd, key)));
	if (occupied(hd, slotOf(hd, key))) return false;

	PutAt(hd, key, val, slotOf(hd, key));
	Occupy(hd, slotOf(hd, key));
	return true;
}
static _Bool TryRehash(struct HashDict* hd, size_t* nSlots) {
//	DEBUG_PRINTF("Rehashing!\n");
	*nSlots = RESIZE(*nSlots);

	struct HashDict* restrict new;
	PALLOCA(new, sizeof(struct HashDict));
	new = HashDictNew(new, *nSlots,
	                  hd->keysize, hd->valsize,
	                  hd->keyHash,
	                  hd->keysEq,
	                  hd->keyfree, hd->valfree);
	sassert(new); // STRICT
	if (!new) return false;

	for (size_t i = 0; i < hd->nSlots; ++i)
		if (occupied(hd, i)) {
			if (!TryAdd(new, KEYIDX(hd, i), VALIDX(hd, i))) {
				/// Failure to rehash: free resources, do not mutate hd.
				new->keyfree = NULL;
				new->valfree = NULL;
				HashDictDelete(new);
				return false; }}
	/* Successful rehash.
	   Free all buffers in hd, but avoid calling {key,val}free. */
	hd->keyfree = NULL;
	hd->valfree = NULL;
	HashDictDelete(hd);

	/* Populate hd. */
	memcpy(hd, new, HashDictSize());
	return true;
}

/* Public Utility Functions: */
/* Simple hash for strings. */
unsigned long cstrcSimpleHash(const void* vps) {
	iassert(vps);                  /* Ensure valid pointer is passed. */
	iassert(*(char* const *) vps); /* Ensure pointer is to a valid string. */
	/* TODO: Set equal to a salt to prevent HT DoS attacks. */
	unsigned long hash = 0;
	const char* s = *(char* const *)vps;
	while(*s)
		hash = hash * 101 + (unsigned long) *s++;
	return hash;
}

_Bool cstrcEq(const void* svp1, const void* svp2) {
	iassert(svp1);
	iassert(svp2);

	const char* const cstr1 = *(const char* const *) svp1;
	const char* const cstr2 = *(const char* const *) svp2;

	iassert(cstr1);
	iassert(cstr2);

	return !strcmp(cstr1, cstr2);
}

/********** Method implementations. **********/

size_t HashDictSize(void)
{ return sizeof(struct HashDict); }

HashDict HashDictNew(void * memory, size_t nSlots,
                     size_t keysize, size_t valsize,
                     size_t (*keyHash)(const void*),
                     _Bool  (*keysEq) (const void*, const void*),
                     /* {key,val}free may be NULL. */
                     void(*keyfree)(void*), void(*valfree)(void*)) {
	sassert(memory);  // STRICT
	/// A zero size is likely programmer error, but may be useful to some.
	/// As such, zero sizes are not preconditions, but guarded against anyhow.
	sassert(keysize); // STRICT
	sassert(valsize); // STRICT

	/// It is impossible for a hash table to operate without these functions.
	/// As such, their validity is a precondition.
	cassert(keyHash);
	cassert(keysEq);
	if (!memory) return NULL;

	if (!nSlots) nSlots = DEFAULT_NSLOTS;

	struct HashDict* hdict = memory;

	/* Allocate buffer. */
	hdict->buf = malloc(nSlots * (keysize + valsize));
	sassert(hdict->buf); // STRICT
	if (!hdict->buf) return NULL;

	/* Initialize slot-tracking BitSet. */
	if(!BitSetInit(&hdict->usedIndices, nSlots)) {
		sassert(!("BitSetInit failed in HashDict__New.\n")); // STRICT
		free(hdict->buf);
		return NULL; }

	/* Initialize local variables. */
	hdict->keyfree = keyfree;
	hdict->valfree = valfree;
	hdict->keysize = keysize;
	hdict->valsize = valsize;

	hdict->keyHash = keyHash;
	hdict->keysEq  = keysEq;
	hdict->nSlots  = nSlots;

	return hdict;
}

_Bool HashDictAdd(struct HashDict* hd, const void* key, const void* val) {
	sassert(hd); // STRICT

	/* If the client attempts to add what NULL points to and something,
	   this is likely programmer error, and they will likely fail to free what
	   is pointed to by the non-NULL argument. Passing two NULLs is similarly
	   nonsensical. As such, the precondition to HashDictAdd is that
	   `key` and `val` are expected to be valid. */
	cassert(key);
	cassert(val);
	if (!hd) return false;

	// TODO: Check if Has, and return false on Has (because ownership wasn't taken)
	// TODO: Recalculate the modulo, rather than calling slotOf each iteration.

	unsigned rc; // Rehash count
	size_t hdSlots = hd->nSlots; // Holds state for TryRehash retries.
	for (rc = 0; rc <= REHASH_LIMIT && occupied(hd, slotOf(hd,key)); ++rc) {
	/* "While TryRehash fails and we are still under the hash limit,
	   say that we have attempted to hash one more time." */
		while(!TryRehash(hd, &hdSlots) && rc < REHASH_LIMIT) ++rc;}
	// Note that TryRehash fails when a value already in hd could not
	// be re-added to the extended `hd`, and thus the loop to continue
	// trying, whereas the outer for loop continues to re-attempt if
	// `val` could not be added.
	return TryAdd(hd, key, val);
}

void HashDictRemove(struct HashDict* hd, const void* key)
{
	sassert(hd); // STRICT
	cassert(key);
	if (!hd) return;

	if (HashDictHas(hd, key)) {
		DeleteAt(hd, slotOf(hd,key));
		Vacate(hd, slotOf(hd,key)); }
}
_Bool HashDictGet(const struct HashDict* hd, const void* key,
		  /* valSlot may be NULL. */
		  void* valSlot) {
	sassert(hd); // STRICT
	cassert(key);
	if (!hd) return false;

	if (HashDictHas(hd, key)) {
		if (valSlot)
			memcpy(valSlot, VALIDX(hd, slotOf(hd,key)), hd->valsize);
		return true; }
	return false;
}

_Bool HashDictHas(const struct HashDict* hd, const void* key)
{
	sassert(hd); // STRICT
	cassert(key);
	if (!key) return false;

	return Has(hd, key, slotOf(hd, key));
}

void HashDictDelete(struct HashDict* hd)
{
	sassert(hd); // STRICT
	if (!hd) return;

	/* Optimization: Skip looping if member destructors aren't defined. */
	if (!hd->keyfree && !hd->valfree) goto hdd_DestructMembers;

	/* Destruct elements that are still owned. */
	for (size_t idx = 0; idx < hd->nSlots; ++idx)
		if (occupied(hd, idx))
			DeleteAt(hd, idx);
hdd_DestructMembers:
	BitSetDelete(&hd->usedIndices);
	free(hd->buf);
}

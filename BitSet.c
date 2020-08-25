#include <stdlib.h> // malloc
#include <string.h> // memset
#include <stdint.h> // SIZE_MAX, uint_least16_t
#include <stdbool.h>

#include "BitSet.h"
#include "Assert.h"

/******************
 * Helper macros. *
 ******************/

/* For assertions */
#define IN_RANGE(bset, telem) (telem < (bset)->P_bound)
#define NO_SIZET_WRAPAROUND(before_cast) ((before_cast) <= SIZE_MAX)

/* For accessing members more easily */
#define GET_BUF(bset)                                                  \
    ( ((bset)->P_heapp) ? (bset)->P_buf.Dynamic : (bset)->P_buf.Static )

/* Returns an index into the word buffer, `bset->P_buf`. */
#define BUFIDX(member) ( (member) / BITSET_WORD_BIT )

/* Returns an index into the bit buffer, `bset->P_buf[BUFIDX(...)]`. */
#define BITIDX(member) ( (member) % BITSET_WORD_BIT )

/********************
 * Helper functions *
 ********************/

/*
 * Tries to table double, otherwise blindly extends to bound.
 * Returns false and changes no state on a failed allocation, else true.
 */
static _Bool Extend(struct BitSet* bset, BITSET_WORD bound) {
    if (bound <= bset->P_bound) return true;

    /* Make sure that the new buffer size fits in a size_t. */
    cassert(NO_SIZET_WRAPAROUND(BITSET_CEILDIV(bound, BITSET_WORD_BIT)));

    /* Try doubling to reduce the frequency of Extend()s, if feasible. */
    if (bound <= (bset->P_bound * 2))
		bound = bset->P_bound * 2;

    size_t newSize = BITSET_CEILDIV(bound, BITSET_WORD_BIT) *
		sizeof(BITSET_WORD);
    size_t oldSize = BITSET_CEILDIV(bset->P_bound, BITSET_WORD_BIT) *
		sizeof(BITSET_WORD);
    BITSET_WORD* oldBuf;
    BITSET_WORD* newBuf = malloc(newSize);

    sassert(newBuf); // STRICT
    if (!newBuf) return false;

	/* Set oldBuf */
    if (!bset->P_heapp) {
		oldBuf = bset->P_buf.Static; // Does this break strict aliasing?
		bset->P_heapp = true;
    } else
		oldBuf = bset->P_buf.Dynamic;

    /* Zero out the new memory. */
    memset(newBuf, 0, newSize);
    /* Copy the old bit patterns over. */
    memcpy(newBuf, oldBuf, oldSize);

    bset->P_buf.Dynamic = newBuf;    // Does this break strict aliasing?

	/* Set the new bound. */
	size_t newBound = newSize * CHAR_BIT;
	cassert(newBound < (SIZE_MAX/2));
	/* Produces warning: appears to be either a compiler bug, or the compiler
	   detecting the usage of a bit-field member short one bit of a full size_t.
	   In either, case, we do an assertion to be sure that we are in range
	   before assignment occurs, so assignment is safe, no matter what GCC says.
	*/
	bset->P_bound = newBound;
    return true;
}

/* Returns false on failure. Extends bound if necessary. */
_Bool BitSetInit(struct BitSet* uninitialized, BITSET_WORD bound)
{
    sassert(uninitialized); // STRICT
    if (!uninitialized) return false;

    uninitialized->P_heapp = false;
    uninitialized->P_bound = BITSET_START_BOUND;

    /* Set all bits to zero. */
    memset(&uninitialized->P_buf.Static, 0,
	   sizeof(BITSET_WORD) * BITSET_INITIAL_ALLOCATION);

    return Extend(uninitialized, bound);
}

BITSET_WORD BitSetBound(const struct BitSet* bset)
{
    return bset->P_bound;
}

/* Returns true if `bset` contains `member`. */
_Bool BitSetHas(const struct BitSet* bset, BITSET_WORD member)
{
    // No wraparound allowed: members must be such that indexes don't wrap
    cassert(NO_SIZET_WRAPAROUND(BUFIDX(member)));
    if (!IN_RANGE(bset, member)) return false;

    const BITSET_WORD* buf = GET_BUF(bset);
    size_t bufIdx = BUFIDX(member);

    return (_Bool) ((1)&(buf[bufIdx] >> BITIDX(member)));
}

/* Returns true if `member` is already in `bset` or on a successful add. */
_Bool BitSetAdd(struct BitSet* bset, BITSET_WORD member)
{
    // No wraparound allowed: members must be such that indexes don't wrap
    cassert(NO_SIZET_WRAPAROUND(BUFIDX(member)));
    if (!IN_RANGE(bset, member))
	if (!Extend(bset, member+1)) {
	    sassert(!("BitSetAdd: Implicit extension failed.")); // STRICT
	    return false; }
    BITSET_WORD* buf = GET_BUF(bset);
    size_t bufIdx = BUFIDX(member);

    buf[bufIdx] |= ( ((BITSET_WORD)1) << BITIDX(member));
    return true;
}

/* Removes `member` from `bset`. */
void BitSetRemove(struct BitSet* bset, BITSET_WORD member)
{
    // No wraparound allowed: members must be such that indexes don't wrap
    cassert(NO_SIZET_WRAPAROUND(BUFIDX(member)));
    if (!IN_RANGE(bset, member)) return;

    BITSET_WORD* buf = GET_BUF(bset);
    size_t bufIdx = BUFIDX(member);

    buf[bufIdx] ^= buf[bufIdx] & ( ((BITSET_WORD)1) << BITIDX(member));
}

/* Destroys a BitSet object by deallocating any buffers. */
void BitSetDelete(struct BitSet* bset)
{
    if (bset->P_heapp) {
		iassert(bset->P_buf.Dynamic);
		free(bset->P_buf.Dynamic); }
}

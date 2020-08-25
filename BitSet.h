#ifndef BITSET_H
#define BITSET_H
#include <stdint.h> // uint_fast16_t
#include <limits.h> // CHAR_BIT

/***********
 * PRIVATE *
 ***********/

#define BITSET_CEILDIV(x, y) ( ((x) + ((y) - 1)) / y )
#define BITSET_WORD uint_fast16_t
#define BITSET_WORD_BIT ( sizeof(BITSET_WORD) * CHAR_BIT )
#define BITSET_START_BOUND 512
#define BITSET_INITIAL_ALLOCATION                                       \
    /* Similar to: ceil((float)BITSET_START_BOUND / BITSET_WORD_BIT) */ \
    BITSET_CEILDIV(BITSET_START_BOUND, BITSET_WORD_BIT)

struct BitSet {
	union {
		BITSET_WORD  Static[BITSET_INITIAL_ALLOCATION];
		BITSET_WORD* Dynamic;
	}P_buf;

	size_t P_heapp : 1;
	size_t P_bound : (sizeof(size_t)*CHAR_BIT) - 1;
};

/**********
 * PUBLIC *
 **********/

/* Returns false on failure. Extends bound if necessary. */
_Bool BitSetInit(struct BitSet* uninitialized, BITSET_WORD bound);

/* Returns the bound. */
BITSET_WORD BitSetBound(const struct BitSet* bset);

/* Returns true if `bset` contains `member`. */
_Bool BitSetHas(const struct BitSet* bset, BITSET_WORD member);

/* Returns true if `member` is already in `bset` or on a successful add. */
_Bool BitSetAdd(struct BitSet* bset, BITSET_WORD member);

/* Removes `member` from `bset`. */
void BitSetRemove(struct BitSet* bset, BITSET_WORD member);

/* Destroys a BitSet object by deallocating any buffers. */
void BitSetDelete(struct BitSet* bset);

#endif /* BITSET_H */

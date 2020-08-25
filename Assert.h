#ifndef ASSERT_H
#define ASSERT_H

#include <assert.h>

/*
 * Asserts that aren't efficient and should be used in debugging contexts only.
 * Meant, in part, for debugging a class implementation.
 */
#ifndef NDEBUG_ASSERT
#define dassert(pred) assert(pred)
#else
#define dassert(pred)
#endif /* NDEBUG_ASSERT */

/*
 * Asserts that are reasonably efficient and check invariants.
 * These asserts are meant for debugging a class implementation.
 * Once a class is implemented correctly, these should be able to be disabled with no outward effect.
 * In effect, these asserts say nothing about an outward facing implementation,
 * but ensure the integrity of its internal details.
 */
#ifndef NINVARIANT_ASSERT
#define iassert(pred) assert(pred)
#else
#define iassert(pred)
#endif /* NINVARIANT_ASSERT */

/*
 * Asserts that impose stricter restrictions than what an API requires.
 */
#ifndef NSTRICT_ASSERT
#define sassert(pred) assert(pred)
#else
#define sassert(pred)
#endif /* NSTRICT_ASSERT */

/*
 * Asserts that impose API requirements or preconditions.
 */
#ifndef NCLIENT_ASSERT
#define cassert(pred) assert(pred)
#else
#define cassert(pred)
#endif /* NCLIENT_ASSERT */

#endif /* ASSERT_H */

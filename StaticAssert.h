#ifndef STATICASSERT_H
#define STATICASSERT_H

/// CONSTEXPR: Assign lval to epxr, and fail to compile if lval isn't
/// a constant expression.
// We just have to hope that the switch is optimized away.
// CONSTEXPR can be very useful in creating macros that implement
// advanced invariant checks done at compile time.
// For example, one can determine whether something is NULL or not NULL
// using CONSTEXPR and STATIC_ASSERT, or possibly just STATIC_ASSERT.
// In this way, it can be possible to 'protect' function calls with
// compile time checks, in ways that restrict function call use.
#define CONSTEXPR(lval, expr) switch(0){ case ((int)(expr)):default:(lval)=(expr); }

#if __STDC_VERSION__ >= 201112L // If C11 is available.
#define STATIC_ASSERT(predicate,message) _Static_assert((predicate), #(message))
#else
#define STATIC_ASSERT(predicate, message)	   \
	STATIC_ASSERT_(predicate, message, __LINE__)
#define STATIC_ASSERT_(predicate, message, ln)	\
	STATIC_ASSERT__((predicate), message, ln)
#define STATIC_ASSERT__(predicate, message, lno)    \
    typedef struct {                                \
        int static_assertion_                       \
        ## message ## _failed_on_line_ ## lno       \
        : !!(predicate); }                          \
        static_assertion_failed_ ## message
#endif // __STDC_VERSION__...

#endif // STATICASSERT_H

// Usage:
// STATIC_ASSERT(2 < 3, two_must_be_less_than_three);

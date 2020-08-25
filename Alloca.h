#ifndef ALLOCA_H
#define ALLOCA_H

/* Portable alloca; Attempts to use VLAs before falling back to alloca. */
/* VLAs are standard in C89, C90, and C99, and are optional in C11.
   alloca is nonstandard, but very common. The standard VLA is preferred,
   except where it does not exist. */

#if __STDC_NO_VLA__
#include <alloca.h>       /* Hope alloca exists. */
#define PALLOCA(lval, size) (lval) = alloca((size))

#else /* __STDC_NO_VLA__ */

// Use VLAs
#define PALLOCA(lval, size)	      \
	char lval ## __MEMORY__[(size)];  \
	(lval) = (void*) lval ## __MEMORY__

#endif /* __STDC_NO_VLA__ */

#endif /* ALLOCA_H */

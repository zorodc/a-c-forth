#ifndef STRDUP_H
#define STRDUP_H
#include <string.h>
/********************
 * Portable strdup  *
 ********************/

#if _XOPEN_SOURCE >= 500 || _POSIX_C_SOURCE >= 200809L ||\
    _BSD_SOURCE || _SVID_SOURCE
#define STRDUP_AVAILABLE
#endif /* ... */

#ifdef STRDUP_AVAILABLE
#define pstrndup strndup
#define pstrdup strdup
#endif /* STRDUP_AVAILABLE */

#ifndef STRDUP_AVAILABLE
#include <stdlib.h> // malloc
char* pstrdup_(const char* s);
char* pstrndup_(const char* s, size_t n);
#define pstrdup pstrdup_
#define pstrndup pstrndup_

#endif /* STRDUP_AVAILABLE */

#endif /* STRDUP_H */

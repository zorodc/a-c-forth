#include "Strdup.h"
#ifndef STRDUP_AVAILABLE
char* pstrdup_(const char* s) {
	char* buf = malloc(strlen(s)+1); /* +1: strlen does not account for '\0' */
	if (!buf) return NULL;
	strcpy(buf, s);
	return buf; }
char* pstrndup_(const char* s, size_t n) {
	char* buf = malloc(n+1);
	strncpy(buf, s, n);
	buf[n]='\0';
	return buf; }
#endif /* STRDUP_AVAILABLE */

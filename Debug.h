#ifndef DEBUG_H
#define DEBUG_H

#ifndef DEBUG_OUT_STREAM
#define DEBUG_OUT_STREAM stderr
#endif /* DEBUG_OUT_STREAM */

#ifdef DEBUG
#include <stdio.h>
#define DEBUG_PRINT(string) (fprintf(DEBUG_OUT_STREAM, "DEBUG: %s", (string)))
#define DEBUG_PRINTF(...)	                            \
	do {fprintf(DEBUG_OUT_STREAM, "DEBUG: ");	        \
	    fprintf(DEBUG_OUT_STREAM, __VA_ARGS__); }while(0)
#define DEBUG_DO(block) do {block}while(0)
#endif /* DEBUG */

#ifndef DEBUG
#define DEBUG_PRINT(string)
#define DEBUG_PRINTF(...)
#define DEBUG_DO(block)
#endif /* DEBUG */

#endif /* DEBUG_H */

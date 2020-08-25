#ifndef GETOBJ_H
#define GETOBJ_H

#include <stdio.h>
#include "Eval.h"

typedef enum object_type(*GetObjFN)(Object*);

/* Takes a FILE* and returns a function pointer to a GetObj. */
/* Only one instance can be used at once, because there are no closures in C. */
GetObjFN CreateGetObj(FILE* stream);

#endif /* GETOBJ_H */

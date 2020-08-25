#ifndef BUILTINS_H
#define BUILTINS_H
#include "Dict.h"

/* Returns false on failure, true otherwise. */
_Bool ImportBuiltins(Dict namespace_to_mutate, _Bool typing_onp);

#endif /* BUILTINS_H */

#ifndef CLEAN_LEAKS_H
#define CLEAN_LEAKS_H

#include <stdio.h>
#include "Stack.h"

void CleanLeaks(Stack main, Stack types,
                // warn can be NULL
                FILE* warn);

#endif /* CLEAN_LEAKS_H */

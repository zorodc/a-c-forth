#include <stdbool.h>
#include <stdlib.h> // free
#include <stdio.h>

#include "CleanLeaks.h"
#include "ForthTypes.h"
#include "Assert.h"

void CleanLeaks(Stack main, Stack types,
                // warn can be NULL
                FILE* warn)
{
	_Bool any = false;
	while (!StackIsEmpty(main) && !StackIsEmpty(types)) {
		any = true;
		enum datum_type itype;
		ForthDatum fd;
		StackPop(types, &itype);
		switch (itype) {
		case T_INT:
			StackPop(main, &fd);
			break;
		case T_STRING:
			StackPop(main, &fd);
			free(fd.String);
			break;
		default:
			fprintf(stderr, "BUG: Unhandled or bad enum datum_type instance %d "
			                "passed to Cleanleaks.\n", itype);
			break;
		}}
	if (any && warn)
		fprintf(warn, "Leak Detected (and cleaned): "
		              "Items left on the stack after execution.\n");
	/* TODO: Print out types of items. */

	if (StackIsEmpty(main) != StackIsEmpty(types))
		fprintf(stderr, "BUG: Unequal number of items "
		                "in the main and typestack passed to CleanLeaks.\n");
}

#ifndef STACK_H
#define STACK_H
typedef struct Stack* Stack;

size_t StackSize(void);

Stack StackNew(void* memory, size_t elemsize,
               // elemfree can be NULL.
               void(*elemfree)(void*));
_Bool StackIsEmpty(Stack s);
_Bool StackPush(Stack s, const void* elem);
_Bool StackPop(Stack s,
               // elemSpace can be NULL.
               void* elemSpace);
/* Return backing buffer. */
void* StackPeek(const Stack s);

void StackDelete(Stack s);

#endif /* STACK_H */

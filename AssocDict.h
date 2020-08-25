#ifndef ASSOCDICT_H
#define ASSOCDICT_H
#include <stddef.h>
typedef struct AssocDict* AssocDict;

#define ASSOCDICT_METHOD_(pref, method_name) pref ## method_name
#define ASSOCDICT_METHOD__(pref, method_name)	\
    ASSOCDICT_METHOD_(pref, method_name)
#define ASSOCDICT_METHOD(method_name)	        \
    ASSOCDICT_METHOD__(ASSOCDICT_PREFIX, method_name)

size_t AssocDictSize(void);

AssocDict AssocDictNew(void* memory, size_t keysize, size_t valsize,
	         _Bool(*keyEq)(const void*, const void*),
	          // [key,val]free can be NULL.
	          void(*keyfree)(void*), void(*valfree)(void*));

_Bool AssocDictAdd(struct AssocDict* d, const void* key, const void* val);

_Bool AssocDictHas(const struct AssocDict* d, const void* key);

_Bool AssocDictGet(const struct AssocDict* d, const void* key,
	               // valSpace can be NULL
	               void* valSpace);

void AssocDictDelete(struct AssocDict* d);

#ifdef ASSOCDICT_PREFIX

typedef AssocDict ASSOCDICT_PREFIX;

static size_t    (* const ASSOCDICT_METHOD(Size))(void) = AssocDictSize;
static AssocDict (* const ASSOCDICT_METHOD(New))(
	                void * memory,
	                size_t keysize, size_t valsize,
	                _Bool(*keyEq  )(const void*, const void*),
	                void (*keyfree)(void*), void(*valfree)(void*)) = AssocDictNew;

static _Bool (* const ASSOCDICT_METHOD(Add))(struct AssocDict* d,
	            const void* key,
	            const void* val) = AssocDictAdd;
static _Bool (* const ASSOCDICT_METHOD(Has))(const struct AssocDict* d,
	            const void* key) = AssocDictHas;
static _Bool (* const ASSOCDICT_METHOD(Get))(const struct AssocDict* d,
	            const void* key,
	            void* valSpace) = AssocDictGet;
static void  (* const ASSOCDICT_METHOD(Delete))(
	           struct AssocDict* d) = AssocDictDelete;

#endif /* ASSOCDICT_PREFIX */
#endif /* ASSOCDICT_H */

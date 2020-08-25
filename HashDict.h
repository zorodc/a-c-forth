#ifndef HASHDICT_H
#define HASHDICT_H

typedef struct HashDict* HashDict;

#define HASHDICT_METHOD_(pref, method_name) pref ## method_name
#define HASHDICT_METHOD__(pref, method_name) HASHDICT_METHOD_(pref, method_name)
#define HASHDICT_METHOD(method_name)	          \
	HASHDICT_METHOD__(HASHDICT_PREFIX, method_name)

unsigned long cstrcSimpleHash(const void* vps);
_Bool cstrcEq(const void* svp1, const void* svp2);

size_t HashDictSize(void);
HashDict HashDictNew(void* memory, size_t nSlots,
                     size_t keysize, size_t valsize,
                     size_t (*keyHash)(const void*),
                     _Bool  (*keysEq) (const void*, const void*),
                     /* {key,val}free may be NULL. */
                     void(*keyfree)(void*), void(*valfree)(void*));

_Bool HashDictAdd(struct HashDict* hd, const void* key, const void* val);
void HashDictRemove(struct HashDict* hd, const void* key);
_Bool HashDictGet(const struct HashDict* hd, const void* key,
                  /* valSlot may be NULL. */
                  void* valSlot);
_Bool HashDictHas(const struct HashDict* hd, const void* key);
void HashDictDelete(struct HashDict* hd);

#ifdef HASHDICT_PREFIX

typedef HashDict HASHDICT_PREFIX;

static size_t   (* const HASHDICT_METHOD(Size))(void) = HashDictSize;
static HashDict (* const HASHDICT_METHOD(New))(void* memory, size_t nSlots,
                                               size_t keysize, size_t valsize,
                                               size_t (*keyHash)(const void*),
                                               _Bool  (*keysEq) (const void*,
                                                                 const void*),
                                               /* {key,val}free may be NULL. */
                                               void(*keyfree)(void*),
                                               void(*valfree)(void*)) = HashDictNew;
static _Bool (* const HASHDICT_METHOD(Add))(struct HashDict* hd,
                                            const void* key,
                                            const void* val) = HashDictAdd;
static void (* const HASHDICT_METHOD(Remove))(struct HashDict* hd,
                                              const void* key) = HashDictRemove;
static _Bool (* const HASHDICT_METHOD(Get))(const struct HashDict* hd,
                                            const void* key,
                                            /* valSlot may be NULL. */
                                            void* valSlot) = HashDictGet;
static _Bool (* const HASHDICT_METHOD(Has))(const struct HashDict* hd,
                                            const void* key) = HashDictHas;
static void (* const HASHDICT_METHOD(Delete))(struct HashDict* hd) = HashDictDelete;
#endif /* HASHDICT_PREFIX */

#endif /* HASHDICT_H */

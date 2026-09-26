#ifndef KINETRA_GC_H
#define KINETRA_GC_H

#include <stdbool.h>
#include <stddef.h>

// Forward declaration to avoid circular includes
typedef struct KValue KValue;

typedef enum {
	OBJ_ARRAY,
	OBJ_STRING
} ObjType;

typedef struct Obj {
	ObjType type;
	bool is_marked;
	struct Obj* next;
} Obj;

typedef struct {
	Obj obj;
	int count;
	KValue* elements;
} ObjArray;

typedef struct {
	Obj obj;
	int length;
	char* chars;
} ObjString;

void gc_init(void);
void gc_free_all(void);

ObjString* gc_alloc_string(const char* chars, int length);
ObjArray* gc_alloc_array(int count);

void gc_collect(void);

void gc_push_root(KValue* KValue);
void gc_pop_root(void);

#endif
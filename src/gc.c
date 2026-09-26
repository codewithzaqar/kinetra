#include "../include/gc.h"
#include "../include/value.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define GC_INITIAL_THRESHOLD (1024 * 1024) // 1MB

typedef struct {
	Obj* objects;
	size_t bytes_allocated;
	size_t next_gc;

	KValue* roots[1024];
	int root_count;

	bool enabled;
	int collections;
} GC;

static GC gc;

void gc_init(void) {
	gc.objects = NULL;
	gc.bytes_allocated = 0;
	gc.next_gc = GC_INITIAL_THRESHOLD;
	gc.root_count = 0;
	gc.enabled = true;
	gc.collections = 0;
}

static void mark_object(Obj* obj) {
	if (!obj || obj->is_marked) return;
	obj->is_marked = true;

	if (obj->type == OBJ_ARRAY) {
		ObjArray* arr = (ObjArray*)obj;
		for (int i = 0; i < arr->count; i++) {
			if (arr->elements[i].type == K_VALUE_ARRAY || arr->elements[i].type == K_VALUE_STRING) {
				mark_object(arr->elements[i].heap);
			}
		}
	}
}

static GcRootScanner root_scanner = NULL;

void gc_set_root_scanner(GcRootScanner scanner) {
	root_scanner = scanner;
}

void gc_mark_value(KValue* value) {
	if (!value) return;

	if (value->type == K_VALUE_ARRAY || value->type == K_VALUE_STRING) {
		mark_object(value->heap);
	}
}

static void mark_roots(void) {
	if (root_scanner) {
		root_scanner();
	}

	for (int i = 0; i < gc.root_count; i++) {
		gc_mark_value(gc.roots[i]);
	}
}

static void sweep(void) {
	Obj** obj = &gc.objects;
	while (*obj) {
		if ((*obj)->is_marked) {
			(*obj)->is_marked = false;
			obj = &(*obj)->next;
		} else {
			Obj* unreached = *obj;
			*obj = unreached->next;

			if (unreached->type == OBJ_STRING) {
				ObjString* str = (ObjString*)unreached;
				gc.bytes_allocated -= sizeof(ObjString) + str->length + 1;
				free(str->chars);
			} else if (unreached->type == OBJ_ARRAY) {
				ObjArray* arr = (ObjArray*)unreached;
				gc.bytes_allocated -= sizeof(ObjArray) + sizeof(KValue) * arr->count;
				free(arr->elements);
			}
			free(unreached);
		}
	}
}

void gc_collect(void) {
	mark_roots();
	sweep();
	gc.next_gc = gc.bytes_allocated * 2;
	gc.collections++;
}

void gc_push_root(KValue* value) {
	if (gc.root_count < 1024) {
		gc.roots[gc.root_count++] = value;
	}
}

void gc_pop_root(void) {
	if (gc.root_count > 0) gc.root_count--;
}

static Obj* allocate_object(size_t size, ObjType type) {
	Obj* obj = (Obj*)malloc(size);
	obj->type = type;
	obj->is_marked = false;

	obj->next = gc.objects;
	gc.objects = obj;

	gc.bytes_allocated += size;

	// Note: Automatic triggering is disabled for a01 to prevent sweeping
	// unrooted object during VM evaluation. Hooked up fully in a02.

	return obj;
}

ObjString* gc_alloc_string(const char* chars, int length) {
	ObjString* str = (ObjString*)allocate_object(sizeof(ObjString), OBJ_STRING);
	str->length = length;
	str->chars = (char*)malloc(length + 1);
	if (chars && length > 0) {
		memcpy(str->chars, chars, length);
	}
	str->chars[length] = '\0';
	gc.bytes_allocated += length + 1;
	return str;
}

ObjArray* gc_alloc_array(int count) {
	ObjArray* arr = (ObjArray*)allocate_object(sizeof(ObjArray), OBJ_ARRAY);
	arr->count = count;
	if (count > 0) {
		arr->elements = (KValue*)calloc(count, sizeof(KValue));
		gc.bytes_allocated += sizeof(KValue) * count;
	} else {
		arr->elements = NULL;
	}
	return arr;
}

void gc_free_all(void) {
	Obj* obj = gc.objects;
	while (obj) {
		Obj* next = obj->next;
		if (obj->type == OBJ_STRING) {
			free(((ObjString*)obj)->chars);
		} else if (obj->type == OBJ_ARRAY) {
			free(((ObjArray*)obj)->elements);
		}
		free(obj);
		obj = next;
	}
	gc.objects = NULL;
}

void gc_set_enabled(bool enabled) {
	gc.enabled = enabled;
}

void gc_try_collect(void) {
	if (!gc.enabled) return;
	if (gc.bytes_allocated <= gc.next_gc) return;

	gc_collect();
}

size_t gc_bytes_allocated(void) {
	return gc.bytes_allocated;
}

int gc_collections(void) {
	return gc.collections;
}
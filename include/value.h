#ifndef KINETRA_VALUE_H
#define KINETRA_VALUE_H

#include <stdbool.h>

typedef enum {
	K_VALUE_NUMBER,
	K_VALUE_VEC3,
	K_VALUE_BOOL,
	K_VALUE_MAT4,
	K_VALUE_ARRAY,
	K_VALUE_PARTICLE,
	K_VALUE_STRING
} KValueType;

typedef struct KValue KValue;

struct KValue {
	KValueType type;

	double number;

	double x;
	double y;
	double z;

	bool boolean;

	double m[16];

	KValue* elements;
	int element_count;
	char* string;

	double px;
	double py;
	double pz;

	double vx;
	double vy;
	double vz;

	double fx;
	double fy;
	double fz;

	double mass;
};

#endif
#ifndef HPC_MATH_H
#define HPC_MATH_H

#include "kinetra.h"

// High-Performance Vector3 Struct (SIMD aligned in future versions)
typedef struct {
	double x, y, z;
} Vec3;

// Matrix 4x4 for transformations
typedef struct {
	double m[16];
} Mat4;

// Math Operations
Vec3 vec3_add(Vec3 a, Vec3 b);
Vec3 vec3_scale(Vec3 v, double scalar);
double vec3_dot(Vec3 a, Vec3 b);

// Simulation Functions
// Euler integration step for physics simulations
Vec3 sim_integrate_euler(Vec3 position, Vec3 velocity, double dt);

// Initialize HPC subsystem
void init_hpc_subsystem(void);

#endif
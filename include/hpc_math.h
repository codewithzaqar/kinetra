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

// additions
Vec3 vec3_cross(Vec3 a, Vec3 b);
double vec3_length(Vec3 v);
Vec3 vec3_normalize(Vec3 v);

// Simulation Functions
// Euler integration step for physics simulations
Vec3 sim_integrate_euler(Vec3 position, Vec3 velocity, double dt);

// Matrix 4x4 operations
Mat4 mat4_identity(void);
Mat4 mat4_translate(double x, double y, double z);
Mat4 mat4_scale(double x, double y, double z);
Mat4 mat4_rotate(Vec3 axis, double angle);
Mat4 mat4_mul(Mat4 a, Mat4 b);
Vec3 mat4_transform_point(Mat4 m, Vec3 v);

// Initialize HPC subsystem
void init_hpc_subsystem(void);

#endif
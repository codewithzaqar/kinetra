#include "../include/hpc_math.h"
#include <math.h>

void init_hpc_subsystem(void) {
    // Placeholder for initializing OpenMP threads, CUDA contexts, 
    // or AVX/SIMD register states for HPC workloads.
    printf("[HPC] Math & Simulation Subsystem initialized.\n");
}

Vec3 vec3_add(Vec3 a, Vec3 b) {
    Vec3 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    return result;
}

Vec3 vec3_scale(Vec3 v, double scalar) {
    Vec3 result;
    result.x = v.x * scalar;
    result.y = v.y * scalar;
    result.z = v.z * scalar;
    return result;
}

double vec3_dot(Vec3 a, Vec3 b) {
    return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
}

Vec3 vec3_cross(Vec3 a, Vec3 b) {
    Vec3 result;
    result.x = a.y * b.z - a.z * b.y;
    result.y = a.z * b.x - a.x * b.z;
    result.z = a.x * b.y - a.y * b.x;
    return result;
}

double vec3_length(Vec3 v) {
    return sqrt(vec3_dot(v, v));
}

Vec3 vec3_normalize(Vec3 v) {
    double len = vec3_length(v);

    if (len == 0.0) {
        Vec3 zero = {0.0, 0.0, 0.0};
        return zero;
    }

    return vec3_scale(v, 1.0 / len);
}

Vec3 sim_integrate_euler(Vec3 position, Vec3 velocity, double dt) {
    // Basic Euler Integration: p' = p + v * dt
    Vec3 scaled_vel = vec3_scale(velocity, dt);
    return vec3_add(position, scaled_vel);
}
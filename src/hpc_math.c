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

Mat4 mat4_identity(void) {
    Mat4 m;

    for (int i = 0; i < 16; i++) {
        m.m[i] = 0.0;
    }

    m.m[0] = 1.0;
    m.m[5] = 1.0;
    m.m[10] = 1.0;
    m.m[15] = 1.0;

    return m;
}

Mat4 mat4_translate(double x, double y, double z) {
    Mat4 m = mat4_identity();

    m.m[3] = x;
    m.m[7] = y;
    m.m[11] = z;

    return m;
}

Mat4 mat4_scale(double x, double y, double z) {
    Mat4 m = mat4_identity();

    m.m[0] = x;
    m.m[5] = y;
    m.m[10] = z;

    return m;
}

Mat4 mat4_rotate(Vec3 axis, double angle) {
    Vec3 k = vec3_normalize(axis);

    double c = cos(angle);
    double s = sin(angle);
    double t = 1.0 - c;

    Mat4 m = mat4_identity();

    m.m[0] = t * k.x * k.x + c;
    m.m[1] = t * k.x * k.y - s * k.z;
    m.m[2] = t * k.x * k.z + s * k.y;

    m.m[4] = t * k.x * k.y + s * k.z;
    m.m[5] = t * k.y * k.y + c;
    m.m[6] = t * k.y * k.z - s * k.x;

    m.m[8] = t * k.x * k.z - s * k.y;
    m.m[9] = t * k.y * k.z + s * k.x;
    m.m[10] = t * k.z * k.z + c;

    return m;
}

Mat4 mat4_mul(Mat4 a, Mat4 b) {
    Mat4 r;

    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 4; col++) {
            double sum = 0.0;

            for (int k = 0; k < 4; k++) {
                sum += a.m[row * 4 + k] * b.m[k * 4 + col];
            }

            r.m[row * 4 + col] = sum;
        }
    }

    return r;
}

Vec3 mat4_transform_point(Mat4 m, Vec3 v) {
    Vec3 r;

    r.x = m.m[0] * v.x + m.m[1] * v.y + m.m[2] * v.z + m.m[3];
    r.y = m.m[4] * v.x + m.m[5] * v.y + m.m[6] * v.z + m.m[7];
    r.z = m.m[8] * v.x + m.m[9] * v.y + m.m[10] * v.z + m.m[11];

    double w = m.m[12] * v.x + m.m[13] * v.y + m.m[14] * v.z + m.m[15];

    if (w != 0.0 && w != 1.0) {
        r.x /= w;
        r.y /= w;
        r.z /= w;
    }

    return r;
}
#pragma once

#include "cl_include.h"

#include "glm/glm.hpp"

template <typename T>
inline cl_float3 to_cl_float3(const T& t) {
    return cl_float3{{t.x, t.y, t.z, 0}};
}

template <typename T>
inline cl_int3 to_cl_int3(const T& t) {
    return cl_int3{{t.x, t.y, t.z, 0}};
}

template <typename T>
inline glm::vec3 to_vec3(const T& t) {
    return glm::vec3(t.x, t.y, t.z);
}

template <>
inline glm::vec3 to_vec3(const cl_float3& t) {
    return glm::vec3(t.s[0], t.s[1], t.s[2]);
}

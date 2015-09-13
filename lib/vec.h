#pragma once

#include <functional>
#include <numeric>
#include <cmath>
#include <ostream>

template <typename T>
struct Vec3;

using Vec3f = Vec3<float>;
using Vec3d = Vec3<double>;
using Vec3i = Vec3<int>;
using Vec3b = Vec3<bool>;

template <typename T>
Vec3<T> make_vec(const T& x, const T& y, const T& z);

template <typename T>
struct Vec3 {
    using value_type = T;
    using Vec3t = Vec3<T>;

    Vec3(T t = T())
            : x(t)
            , y(t)
            , z(t) {
    }

    Vec3(const T& x, const T& y, const T& z)
            : x(x)
            , y(y)
            , z(z) {
    }

    template <typename U>
    Vec3(const Vec3<U>& u)
            : x(u.x)
            , y(u.y)
            , z(u.z) {
    }

    template <typename V, typename U>
    auto fold(const U& u = U(), const V& v = V()) const {
#ifdef __APPLE__
        return v(x, v(y, v(z, u)));
#else
        return std::accumulate(std::begin(s), std::end(s), u, v);
#endif
    }

    template <typename U>
    auto map(const U& u = U()) const {
        return make_vec(u(x), u(y), u(z));
    }

    template<typename... U>
    auto zip(const U&... u) const {
        return make_vec(std::make_tuple(x, u.x...),
                        std::make_tuple(y, u.y...),
                        std::make_tuple(z, u.z...));
    }

    template <typename U>
    auto apply(const Vec3& rhs, const U& u = U()) const {
        return zip(rhs).map(
            [&u](const auto& i) { return u(std::get<0>(i), std::get<1>(i)); });
    }

    template <typename U, typename V>
    auto apply(const Vec3 & a, const Vec3& b, const U & u = U()) const {
        return zip(a, b).map(
            [&u](const auto & i) { return u(std::get<0>(i), std::get<1>(i), std::get<2>(i)); });
    }

    T sum() const {
        return fold<std::plus<T>>(0);
    }

    T product() const {
        return fold<std::multiplies<T>>(1);
    }

    T mag_squared() const {
        return dot(*this);
    }

    T mag() const {
        return sqrt(mag_squared());
    }

    template <typename U>
    auto dot(const Vec3<U>& rhs) const {
        return (*this * rhs).sum();
    }

    template <typename U>
    auto cross(const Vec3<U>& rhs) const {
        return make_vec(y * rhs.z - z * rhs.y,
                        z * rhs.x - x * rhs.z,
                        x * rhs.y - y * rhs.x);
    }

    template <typename U>
    Vec3b operator==(const Vec3<U>& rhs) const {
        return apply<std::equal_to<std::common_type_t<T, U>>>(rhs);
    }

    template <typename U>
    Vec3b operator==(const U& rhs) const {
        return operator==(Vec3<U>(rhs));
    }

    template <typename U>
    Vec3b operator<(const Vec3<U>& rhs) const {
        return apply<std::less<std::common_type_t<T, U>>>(rhs);
    }

    template <typename U>
    Vec3b operator>(const Vec3<U>& rhs) const {
        return apply<std::greater<std::common_type_t<T, U>>>(rhs);
    }

    template <typename U>
    Vec3b operator&&(const Vec3<U>& rhs) const {
        return apply<std::logical_and<std::common_type_t<T, U>>>(rhs);
    }

    template <typename U>
    Vec3b operator||(const Vec3<U>& rhs) const {
        return apply<std::logical_or<std::common_type_t<T, U>>>(rhs);
    }

    template <typename U>
    auto operator+(const Vec3<U>& rhs) const {
        return apply<std::plus<std::common_type_t<T, U>>>(rhs);
    }

    template <typename U>
    auto operator+(const U& rhs) const {
        return operator+(Vec3<U>(rhs));
    }

    template <typename U>
    auto operator-(const Vec3<U>& rhs) const {
        return apply<std::minus<std::common_type_t<T, U>>>(rhs);
    }

    template <typename U>
    auto operator-(const U& rhs) const {
        return operator-(Vec3<U>(rhs));
    }

    template <typename U>
    auto operator*(const Vec3<U>& rhs) const {
        return apply<std::multiplies<std::common_type_t<T, U>>>(rhs);
    }

    template <typename U>
    auto operator*(const U& rhs) const {
        return operator*(Vec3<U>(rhs));
    }

    template <typename U>
    auto operator/(const Vec3<U>& rhs) const {
        return apply<std::divides<std::common_type_t<T, U>>>(rhs);
    }

    template <typename U>
    auto operator/(const U& rhs) const {
        return operator/(Vec3<U>(rhs));
    }

    bool all() const {
        return fold<std::logical_and<T>>(true);
    }

    bool any() const {
        return fold<std::logical_or<T>>(false);
    }

#ifdef __APPLE__
    T x, y, z;
#else
    union {
        T s[3];
        struct {
            T x, y, z;
        };
    };
#endif
};

template <typename T>
Vec3<T> make_vec(const T& x, const T& y, const T& z) {
    return Vec3<T>(x, y, z);
}

template <typename T>
std::ostream& operator<<(std::ostream& strm, const Vec3<T>& obj) {
    return strm << "(" << obj.x << ", " << obj.y << ", " << obj.z << ")";
}

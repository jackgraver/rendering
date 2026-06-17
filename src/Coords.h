#ifndef COORDS_H
#define COORDS_H

#include <cstddef>
#include <functional>

#include <glm/glm.hpp>

struct IVec3Hash {
    std::size_t operator()(const glm::ivec3& v) const noexcept {
        std::size_t x = std::hash<int>{}(v.x);
        std::size_t y = std::hash<int>{}(v.y);
        std::size_t z = std::hash<int>{}(v.z);

        return x ^ (y << 1) ^ (z << 2);
    }
};

struct Coords {
    glm::ivec3 value = glm::ivec3(0);

    Coords() = default;
    Coords(int x, int y, int z) noexcept
        : value(x, y, z) {
    }

    Coords(const glm::ivec3& v) noexcept
        : value(v) {
    }

    explicit Coords(const glm::vec3& v) noexcept
        : value(static_cast<int>(v.x), static_cast<int>(v.y), static_cast<int>(v.z)) {
    }

    int& x() noexcept { return value.x; }
    const int& x() const noexcept { return value.x; }

    int& y() noexcept { return value.y; }
    const int& y() const noexcept { return value.y; }

    int& z() noexcept { return value.z; }
    const int& z() const noexcept { return value.z; }

    glm::ivec3& vec() noexcept { return value; }
    const glm::ivec3& vec() const noexcept { return value; }

    operator const glm::ivec3&() const noexcept {
        return value;
    }

    operator glm::ivec3&() noexcept {
        return value;
    }

    explicit operator glm::vec3() const noexcept {
        return glm::vec3(value);
    }

    Coords operator+(const Coords& other) const noexcept {
        return Coords(value + other.value);
    }

    Coords operator-(const Coords& other) const noexcept {
        return Coords(value - other.value);
    }

    Coords operator+(const glm::ivec3& other) const noexcept {
        return Coords(value + other);
    }

    Coords operator-(const glm::ivec3& other) const noexcept {
        return Coords(value - other);
    }

    Coords& operator+=(const Coords& other) noexcept {
        value += other.value;
        return *this;
    }

    Coords& operator-=(const Coords& other) noexcept {
        value -= other.value;
        return *this;
    }

    bool operator==(const Coords& other) const noexcept {
        return value == other.value;
    }

    bool operator!=(const Coords& other) const noexcept {
        return !(*this == other);
    }
};

struct CoordsHash {
    std::size_t operator()(const Coords& v) const noexcept {
        return IVec3Hash{}(v.value);
    }
};

namespace std {
template <>
struct hash<Coords> {
    std::size_t operator()(const Coords& v) const noexcept {
        return CoordsHash{}(v);
    }
};
} // namespace std

#endif // COORDS_H

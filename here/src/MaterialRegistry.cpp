#include "Constants.hpp"
#include "particles/Particle.hpp"
#include "particles/MovableSolid.hpp"
#include "particles/Gas.hpp"
#include "particles/Liquid.hpp"
#include "particles/ImmovableSolid.hpp"

// 1. Define the creation helper
// Use ... to capture the { color list }
#define MAP_PARTICLE(ClassName, ...) \
    MaterialProps{ \
        MaterialID::ClassName, \
        #ClassName, \
        __VA_ARGS__, \
        ClassName::getStaticGroup(), \
        []() { return std::make_unique<ClassName>(); } \
    }

// 2. Define the logic for the "X" macro
#define X_EXPAND(name, ...) MAP_PARTICLE(name, __VA_ARGS__),

// 3. Special handling for EmptyParticle
#undef X_EXPAND
#define X_EXPAND(name, ...) \
    std::string(#name) == "EmptyParticle" ? \
    MaterialProps{ MaterialID::name, "Empty", __VA_ARGS__, MaterialGroup::ImmovableSolid, [](){ return nullptr; } } : \
    MAP_PARTICLE(name, __VA_ARGS__),

// 4. Initialize the vector
const std::vector<MaterialProps> ALL_MATERIALS = {
    PARTICLE_DATA(X_EXPAND)
};
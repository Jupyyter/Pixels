#include "Constants.hpp"
#include "particles/Particle.hpp"
#include "particles/MovableSolid.hpp"
#include "particles/Gas.hpp"
#include "particles/Liquid.hpp"
#include "particles/ImmovableSolid.hpp"

// 1. Define the creation helper
// We use a template-like approach to handle EmptyParticle specifically
#define MAP_PARTICLE(ClassName, Color) \
    MaterialProps{ \
        MaterialID::ClassName, \
        #ClassName, \
        Color, \
        ClassName::getStaticGroup(), \
        []() { return std::make_unique<ClassName>(); } \
    }

// 2. Define the logic for the "X" macro
// This checks if the name is 'EmptyParticle' to avoid calling getStaticGroup() on a non-existent class
#define X_EXPAND(name, color) MAP_PARTICLE(name, color),

// 3. If EmptyParticle is NOT a class, we need to override the macro for just that one:
// (If you created a class for EmptyParticle, you can delete these 3 lines)
#undef X_EXPAND
#define X_EXPAND(name, color) \
    std::string(#name) == "EmptyParticle" ? \
    MaterialProps{ MaterialID::name, "Empty", color, MaterialGroup::ImmovableSolid, [](){ return nullptr; } } : \
    MAP_PARTICLE(name, color),

// 4. Initialize the vector (Notice: NO #defines inside the braces!)
const std::vector<MaterialProps> ALL_MATERIALS = {
    PARTICLE_DATA(X_EXPAND)
};
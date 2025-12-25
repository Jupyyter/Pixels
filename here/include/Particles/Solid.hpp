#include "Particle.hpp"
#pragma once
class Solid : public Particle {
public:
    Solid(MaterialID id) : Particle(id) {}
};
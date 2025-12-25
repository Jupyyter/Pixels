#include "Solid.hpp"
#pragma once
class ImmovableSolid : public Solid {
public:
    ImmovableSolid(MaterialID id) : Solid(id) {this->isFreeFalling = false;}
    static MaterialGroup getStaticGroup() { return MaterialGroup::ImmovableSolid; }
    // Even static particles get an update as requested
    void update(int x, int y, float dt, ParticleWorld& world) override {
        // Do nothing by default, or handle specific interactions (like burning)
    }
};
class Stone : public ImmovableSolid {
public:
    Stone() : ImmovableSolid(MaterialID::Stone) {
        // Color randomization logic
    }
    std::unique_ptr<Particle> clone() const override { return std::make_unique<Stone>(*this); }
};
class Wood : public ImmovableSolid {
public:
    Wood() : ImmovableSolid(MaterialID::Wood) { /* Color logic */ }
    std::unique_ptr<Particle> clone() const override { return std::make_unique<Wood>(*this); }
    
    // Wood is static, but creates interactions
    // Note: Usually fire checks for wood, but if wood needs to do something:
    // override update here.
};
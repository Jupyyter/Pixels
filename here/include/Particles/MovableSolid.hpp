#pragma once
#include "Solid.hpp"
#include "Random.hpp"
class MovableSolid : public Solid {
public:
    MovableSolid(MaterialID id) : Solid(id) {
        stoppedMovingThreshold = 5;
        velocity.y = 124.0f; // SFML: +124 is falling Down
    }
    static MaterialGroup getStaticGroup() { return MaterialGroup::MovableSolid; }

    void update(int x, int y, float dt, ParticleWorld& world) override;

protected:
    int stoppedMovingCount = 0;
    int stoppedMovingThreshold = 5;

    virtual bool actOnNeighbor(int targetX, int targetY, int& currentX, int& currentY, ParticleWorld& world, bool isFinal, bool isFirst, int depth);
    void setAdjacentNeighborsFreeFalling(int x, int y, ParticleWorld& world, int depth);
    int getAdditional(float val);
    float getAverageVelOrGravity(float myVel, float otherVel);
};

class Sand : public MovableSolid {
public:
    Sand() : MovableSolid(MaterialID::Sand) {
        velocity.x = (Random::randBool()) ? -1.0f : 1.0f;
        velocity.y = 124.0f; 
        frictionFactor = 0.9f;
        inertialResistance = 0.1f;
        mass = 150;
    }
    std::unique_ptr<Particle> clone() const override { return std::make_unique<Sand>(*this); }
};

class Dirt : public MovableSolid {
public:
    Dirt() : MovableSolid(MaterialID::Dirt) {
        velocity.y = 124.0f;
        frictionFactor = 0.6f;
        inertialResistance = 0.8f;
        mass = 200;
    }
    std::unique_ptr<Particle> clone() const override { return std::make_unique<Dirt>(*this); }
};

class Coal : public MovableSolid {
public:
    Coal() : MovableSolid(MaterialID::Coal) {
        velocity.y = 124.0f;
        frictionFactor = 0.4f;
        inertialResistance = 0.8f;
        mass = 200;
        flammabilityResistance = 100;
        resetFlammabilityResistance = 35;
    }
    std::unique_ptr<Particle> clone() const override { return std::make_unique<Coal>(*this); }
};

class Gunpowder : public MovableSolid {
public:
    int ignitedCount = 0;
    Gunpowder() : MovableSolid(MaterialID::Gunpowder) {
        velocity.y = 124.0f;
        frictionFactor = 0.4f;
        inertialResistance = 0.8f;
        mass = 200;
        flammabilityResistance = 10;
        resetFlammabilityResistance = 35;
    }
    void update(int x, int y, float dt, ParticleWorld& world) override;
    std::unique_ptr<Particle> clone() const override { return std::make_unique<Gunpowder>(*this); }
};

class Snow : public MovableSolid {
public:
    Snow() : MovableSolid(MaterialID::Snow) {
        velocity.y = 62.0f; // Slower fall for snow
        frictionFactor = 0.4f;
        inertialResistance = 0.8f;
        mass = 200;
    }
    void update(int x, int y, float dt, ParticleWorld& world) override;
    std::unique_ptr<Particle> clone() const override { return std::make_unique<Snow>(*this); }
};

class Ember : public MovableSolid {
public:
    Ember() : MovableSolid(MaterialID::Ember) {
        velocity.y = 124.0f;
        frictionFactor = 0.9f;
        inertialResistance = 0.99f;
        mass = 200;
        isIgnited = true;
        health = Random::randInt(250, 350);
    }
    std::unique_ptr<Particle> clone() const override { return std::make_unique<Ember>(*this); }
};
class Salt : public MovableSolid {
public:
    Salt() : MovableSolid(MaterialID::Salt) {
        velocity.y = 124.0f;
        frictionFactor = 0.5f;
        inertialResistance = 0.3f;
    }
    std::unique_ptr<Particle> clone() const override { return std::make_unique<Salt>(*this); }
};
#pragma once
#include "Solid.hpp"
#include "Random.hpp"
#include <cmath>

class MovableSolid : public Solid {
public:
    MovableSolid(MaterialID id) : Solid(id) {
        stoppedMovingThreshold = 5;
        // SFML: Positive Y is down. Gravity accumulates positively.
        velocity.y = 124.0f; 
    }

    static MaterialGroup getStaticGroup() { return MaterialGroup::MovableSolid; }
MaterialGroup getGroup() const override { 
        return getStaticGroup(); 
    }
    void update(int x, int y, float dt, ParticleWorld& world) override;

protected:
    void setAdjacentNeighborsFreeFalling(int x, int y, ParticleWorld& world, int depth);
    int getAdditional(float val);
    float getAverageVelOrGravity(float myVel, float otherVel);
    
    // Core interaction logic ported from Element.java/MovableSolid.java
    bool actOnNeighbor(int targetX, int targetY, int& currentX, int& currentY, 
                       ParticleWorld& world, bool isFinal, bool isFirst, int depth) override;
};

// --- Derived Classes ---

class Sand : public MovableSolid {
public:
    Sand() : MovableSolid(MaterialID::Sand) {
        // Java: Math.random() > 0.5 ? -1 : 1
        velocity.x = (Random::randBool()) ? -1.0f : 1.0f;
        velocity.y = 124.0f; 
        frictionFactor = 0.9f;
        inertialResistance = 0.1f;
        mass = 150;
    }
    
    // Java: Returns false (sand doesn't burn/conduct heat the standard way)
    bool receiveHeat(int heat) override {
        return false;
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

    // Java: Returns false
    bool receiveHeat(int heat) override {
        return false;
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
    
    void spawnSparkIfIgnited(ParticleWorld& world) override;

    std::unique_ptr<Particle> clone() const override { return std::make_unique<Coal>(*this); }
};

class Gunpowder : public MovableSolid {
public:
    int ignitedCount = 0;
    int ignitedThreshold = 7;

    Gunpowder() : MovableSolid(MaterialID::Gunpowder) {
        velocity.y = 124.0f;
        frictionFactor = 0.4f;
        inertialResistance = 0.8f;
        mass = 200;
        flammabilityResistance = 10;
        resetFlammabilityResistance = 35;
        // Java: explosionRadius = 15;
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
        flammabilityResistance = 100;
        resetFlammabilityResistance = 35;
    }

    void update(int x, int y, float dt, ParticleWorld& world) override;
    bool receiveHeat(int heat) override;

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
        // Java: getRandomInt(100) + 250
        health = Random::randInt(250, 350); 
        temperature = 5;
        flammabilityResistance = 0;
        resetFlammabilityResistance = 20;
    }
    
    // Java: infect returns false
    // Note: If you implement infect() in base, override here to return false.

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
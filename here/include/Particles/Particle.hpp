#pragma once
#include <SFML/Graphics.hpp>
#include <memory>
#include "Constants.hpp"
class ParticleWorld;

class Particle {
public:
    MaterialID id;
    sf::Vector2f velocity{0.0f, 0.0f};
    sf::Color color;
    bool hasBeenUpdatedThisFrame = false;
    float lifeTime = 0.0f;
    // Physics & State from element.java
    bool isFreeFalling = true;
    float frictionFactor = 0.5f;
    float inertialResistance = 0.1f;
    int mass = 100;
    int health = 500;
    
    // Thresholds for sub-pixel movement
    float xThreshold = 0.0f;
    float yThreshold = 0.0f;
    
    // Heat/Fire system
    bool isIgnited = false;
    int flammabilityResistance = 100;
    int resetFlammabilityResistance = 50;
    int fireDamage = 3;

    Particle(MaterialID matId) : id(matId), color(GetProps(matId).color) {}
    virtual ~Particle() = default;

    virtual void update(int x, int y, float dt, ParticleWorld& world) = 0;
    virtual std::unique_ptr<Particle> clone() const = 0;
    
    // Ported from Element.java
    virtual bool receiveHeat(int heat) {
        if (isIgnited) return false;
        flammabilityResistance -= (rand() % heat);
        if (flammabilityResistance <= 0) isIgnited = true;
        return true;
    }
};

class EmptyParticle : public Particle {
public:
    EmptyParticle() : Particle(MaterialID::EmptyParticle) {}
static MaterialGroup getStaticGroup() { return MaterialGroup::Special; }
    // Empty particles do absolutely nothing
    void update(int x, int y, float dt, ParticleWorld& world) override { 
        hasBeenUpdatedThisFrame = false; // Keep it reset
    }

    std::unique_ptr<Particle> clone() const override { 
        return std::make_unique<EmptyParticle>(*this); 
    }
};
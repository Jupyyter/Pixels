#include "Particle.hpp"
#include "Random.hpp"
#pragma once
class Liquid : public Particle {
protected:
    int dispersionRate;
    float frictionFactor;
    
    // Sub-pixel Accumulators
    float xThreshold = 0.0f;
    float yThreshold = 0.0f;

public:
    int density; 

    Liquid(MaterialID id, int dispRate, int dens, float friction = 1.0f) 
        : Particle(id), dispersionRate(dispRate), density(dens), frictionFactor(friction) {}
static MaterialGroup getStaticGroup() { return MaterialGroup::Liquid; }
    void update(int x, int y, float dt, ParticleWorld& world) override;

private:
    // Returns TRUE if the particle STOPPED or DISPERSED.
    // Returns FALSE if the particle should continue moving along its vector.
    bool actOnNeighbor(int targetX, int targetY, int& currentX, int& currentY, 
                       ParticleWorld& world, bool isFinal, bool isFirst, int depth);

    // The sliding logic
    bool iterateToAdditional(ParticleWorld& world, int startX, int startY, int distance, int& currentX, int& currentY);
    
    int getAdditional(float val);
    float getAverageVelOrGravity(float myVel, float otherVel);
};
// ... Acid, Lava, Oil remain similar but will inherit the new update ...

class Water : public Liquid {
public:
    Water() : Liquid(MaterialID::Water, 5, 5, 1.0f) {}
    std::unique_ptr<Particle> clone() const override { return std::make_unique<Water>(*this); }
};
class Acid : public Liquid {
public:
    Acid() : Liquid(MaterialID::Acid, 3.0f, 1.3,1.0f) {}
    std::unique_ptr<Particle> clone() const override { return std::make_unique<Acid>(*this); }

    void update(int x, int y, float dt, ParticleWorld& world) override {
        // Dissolve neighbors logic
        
        // Move
        Liquid::update(x, y, dt, world);
    }
};
class Lava : public Liquid {
public:
    // Lava has no specific "spread/density" in the constructor because 
    // we are custom-coding its movement to match your original logic exactly.
    Lava() : Liquid(MaterialID::Lava, 0.0f, 0.0f) {
        // Randomize color slightly on creation
        color.r += Random::randInt(-30, 30);
        color.g += Random::randInt(-10, 10);
    }

    std::unique_ptr<Particle> clone() const override { 
        return std::make_unique<Lava>(*this); 
    }

    void update(int x, int y, float dt, ParticleWorld& world) override;
};
class Oil : public Liquid {
public:
    // Oil: spread 4.0, density 1.5
    Oil() : Liquid(MaterialID::Oil, 4.0f, 2.0f,1.0f) {}
    std::unique_ptr<Particle> clone() const override { return std::make_unique<Oil>(*this); }

    void update(int x, int y, float dt, ParticleWorld& world) override {
        // Check ignition logic
        // If ignited -> turn to fire
        
        // Then move
        Liquid::update(x, y, dt, world);
    }
};

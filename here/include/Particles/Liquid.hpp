#pragma once
#include "Particle.hpp"
#include "Random.hpp"

// --- Base Liquid Class ---
class Liquid : public Particle {
protected:
    int dispersionRate;
    
public:
    int density; 

    Liquid(MaterialID id, int dispRate, int dens, float friction = 1.0f) 
        : Particle(id), dispersionRate(dispRate), density(dens) {
        this->frictionFactor = friction;
        this->stoppedMovingThreshold = 10; // Default from Liquid.java
    }

    static MaterialGroup getStaticGroup() { return MaterialGroup::Liquid; }
    MaterialGroup getGroup() const override { 
        return getStaticGroup(); 
    }
    // Final update logic including the movement tracker from Java
    void update(int x, int y, float dt, ParticleWorld& world) override;

private:
    bool actOnNeighbor(int targetX, int targetY, int& currentX, int& currentY, 
                       ParticleWorld& world, bool isFinal, bool isFirst, int depth) override;

    bool iterateToAdditional(ParticleWorld& world, int startX, int startY, int distance, int& currentX, int& currentY);
    
    int getAdditional(float val);
    float getAverageVelOrGravity(float myVel, float otherVel);
    bool compareDensities(Liquid* neighbor);
    void swapLiquidForDensities(ParticleWorld& world, Liquid* neighbor, int neighborX, int neighborY, int& currentX, int& currentY);
};

// --- Water ---
class Water : public Liquid {
public:
    Water() : Liquid(MaterialID::Water, 5, 5, 1.0f) {
        inertialResistance = 0;
        mass = 100;
        coolingFactor = 5;
        explosionResistance = 0;
    }

    std::unique_ptr<Particle> clone() const override { return std::make_unique<Water>(*this); }

    // Ported from Water.java
    bool receiveHeat(int heat) override {
        // Water dies and becomes Steam
        // Assuming MaterialID::Steam exists
        // dieAndReplace(MaterialID::Steam, world) requires access to world, 
        // but receiveHeat signature in base is just (int heat). 
        // Logic usually handled in update or actOnOther in C++ structure, 
        // but to match Java strictly we might need 'world' in receiveHeat.
        // For now, we flag it dead here or handle logic where receiveHeat is called.
        // *Adaptation*: We return true, caller should handle, OR we assume we can't spawn steam inside this specific function signature without world.
        // *Fix*: We will rely on actOnOther to handle the steam conversion mostly, or update the Base signature in your own time.
        // However, looking at ActOnOther below:
        return true; 
    }

    bool actOnOther(Particle* other, ParticleWorld& world) override {
        other->cleanColor();
        if (other->shouldApplyHeat()) {
            other->receiveCooling(coolingFactor);
            coolingFactor--;
            if (coolingFactor <= 0) {
                dieAndReplace(MaterialID::Steam, world);
                return true;
            }
            return false;
        }
        return false;
    }
    
    bool explode(ParticleWorld& world, int strength) override {
        if (explosionResistance < strength) {
            dieAndReplace(MaterialID::Steam, world);
            return true;
        }
        return false;
    }
};

// --- Oil ---
class Oil : public Liquid {
public:
    // Oil: spread 4.0, density 1.5 (mapped to int density 4 in Java)
    Oil() : Liquid(MaterialID::Oil, 4, 4, 1.0f) {
        mass = 75;
        flammabilityResistance = 5;
        resetFlammabilityResistance = 2;
        fireDamage = 10;
        temperature = 10;
        health = 1000;
    }
    std::unique_ptr<Particle> clone() const override { return std::make_unique<Oil>(*this); }

    // Oil catches fire easily
    bool actOnOther(Particle* other, ParticleWorld& world) override {
        if (other->isIgnited || other->id == MaterialID::Lava) {
            this->receiveHeat(100); // Force ignite
        }
        return false;
    }
};

// --- Lava ---
class Lava : public Liquid {
public:
    int magmatizeDamage;

    Lava() : Liquid(MaterialID::Lava, 1, 10, 1.0f) {
        mass = 100;
        temperature = 10;
        heated = true;
        magmatizeDamage = Random::randInt(0, 10);

    }

    std::unique_ptr<Particle> clone() const override { return std::make_unique<Lava>(*this); }

    // Lava is immune to standard heat
    bool receiveHeat(int heat) override {
        return false; 
    }
    
    // Lava has custom death conditions based on temperature
    void checkIfDead(ParticleWorld& world) override {
        if (isDead) return;

        if (this->temperature <= 0) {
            sf::Vector2i originalPosition = this->position;
            dieAndReplace(MaterialID::Stone, world);

            // Harden neighboring liquids into stone, as in Java
            for (int x = -1; x <= 1; ++x) {
                for (int y = -1; y <= 1; ++y) {
                    if (x == 0 && y == 0) continue;
                    
                    // NOTE: This assumes your ParticleWorld has a 'getParticleAt' method.
                    Particle* neighbor = world.getParticleAt(originalPosition.x + x, originalPosition.y + y);
                    if (neighbor && GetProps(neighbor->id).group == MaterialGroup::Liquid) {
                        neighbor->dieAndReplace(MaterialID::Stone, world);
                    }
                }
            }
            return; // Particle is dead, no further checks needed
        }

        // Fallback to the original health check from the Java version's logic
        if (this->health <= 0) {
            die(world);
        }
    }

    bool actOnOther(Particle* other, ParticleWorld& world) override {
        other->magmatize(world, this->magmatizeDamage);
        return false;
    }

    void magmatize(ParticleWorld& world, int damage) override { 
        // Lava cannot be magmatized
    }

    bool receiveCooling(int cooling) override {
        this->temperature -= cooling;
        // The actual check for turning to stone is now handled in checkIfDead,
        // which is called by the main update loop.
        return true;
    }
};


// --- Acid ---
class Acid : public Liquid {
public:
    int acidStrength = 3; // Mapped from corrosionCount? Java says corrosionCount = 3
    int corrosionCount = 3;

    Acid() : Liquid(MaterialID::Acid, 2, 2, 1.0f) {
        mass = 50;
    }
    std::unique_ptr<Particle> clone() const override { return std::make_unique<Acid>(*this); }

    bool actOnOther(Particle* other, ParticleWorld& world) override {
        
        // Visual stain
        other->stain(sf::Color(0, 255, 0, 100)); // Greenish stain

        // Dissolve interaction
        bool corroded = other->corrode(world);
        if (corroded) {
            corrosionCount--;
            if (corrosionCount <= 0) {
                // Java: dieAndReplace(ElementType.FLAMMABLEGAS);
                dieAndReplace(MaterialID::FlammableGas, world); 
            }
            return true; // Interaction happened
        }
        return false;
    }
    
    bool corrode(ParticleWorld& world) override {
        return false; // Acid doesn't corrode itself
    }
    
    bool receiveHeat(int heat) override {
        return false;
    }
};

// --- Cement ---
class Cement : public Liquid {
public:
    Cement() : Liquid(MaterialID::Cement, 1, 9, 1.0f) {
        inertialResistance = 0;
        mass = 100;
        coolingFactor = 5;
        // In Java, stoppedMovingThreshold = 900
        stoppedMovingThreshold = 50; 
    }

    std::unique_ptr<Particle> clone() const override { return std::make_unique<Cement>(*this); }
    
    // Cement overrides the main update function to add its hardening logic,
    // exactly like the Java version overrides step().
    void update(int x, int y, float dt, ParticleWorld& world) override {
        // First, run all the standard liquid movement and physics.
        Liquid::update(x, y, dt, world);

        // If the particle died during the base update, do nothing more.
        if (isDead) {
            return;
        }

        // After the base update, check if it should harden.
        if (stoppedMovingCount >= stoppedMovingThreshold) {
            dieAndReplace(MaterialID::Stone, world);
        }
    }

    bool receiveHeat(int heat) override { return false; }
    bool actOnOther(Particle* other, ParticleWorld& world) override { return false; }
};
// --- Blood ---
class Blood : public Liquid {
public:
    // Java file provided was a copy of Cement, but standard Blood logic is usually:
    // Similar to water, stains things red.
    Blood() : Liquid(MaterialID::Blood, 5, 6, 1.0f) {
        mass = 100;
        inertialResistance = 0;
        coolingFactor = 3;
    }
    
    std::unique_ptr<Particle> clone() const override { return std::make_unique<Blood>(*this); }

    bool actOnOther(Particle* other, ParticleWorld& world) override {
        other->stain(sf::Color(150, 0, 0)); // Red stain
        
        if (other->shouldApplyHeat()) {
             dieAndReplace(MaterialID::Steam, world);
             return true;
        }
        return false;
    }
};
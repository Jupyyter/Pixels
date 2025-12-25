#include "Particle.hpp"
#include "ParticleWorld.hpp"
#pragma once
class Gas : public Particle {
protected:
    float buoyancy;
    float chaosLevel;
    
public:
    Gas(MaterialID id, float buoy, float chaos) 
        : Particle(id), buoyancy(buoy), chaosLevel(chaos) {}
static MaterialGroup getStaticGroup() { return MaterialGroup::Gas; }
    void update(int x, int y, float dt, ParticleWorld& world) override;
};
class Steam : public Gas {
public:
    // Buoyancy: 1.0f, Chaos: 1.8f
    Steam() : Gas(MaterialID::Steam, 1.0f, 1.8f) {}

    std::unique_ptr<Particle> clone() const override { 
        return std::make_unique<Steam>(*this); 
    }

    void update(int x, int y, float dt, ParticleWorld& world) override ;
};
class Smoke : public Gas {
public:
    Smoke() : Gas(MaterialID::Smoke, 0.8f, 1.2f) {}
    std::unique_ptr<Particle> clone() const override { return std::make_unique<Smoke>(*this); }
    
    void update(int x, int y, float dt, ParticleWorld& world) override {
        if (hasBeenUpdatedThisFrame) return;
        hasBeenUpdatedThisFrame = true;

        // Lifetime Logic
        if (lifeTime > 15.0f) {
            // Remove self (set to Empty in world)
            world.setParticleAt(x, y, nullptr); // or createEmpty
            return;
        }

        // Color fading logic
        // ...

        Gas::update(x, y, dt, world);
    }
};
class Fire : public Gas {
public:
    // Fire acts like gas but has very specific update logic
    Fire() : Gas(MaterialID::Fire, 0.0f, 0.0f) {} 
    std::unique_ptr<Particle> clone() const override { return std::make_unique<Fire>(*this); }

    void update(int x, int y, float dt, ParticleWorld& world) override {
        if (hasBeenUpdatedThisFrame) return;
        hasBeenUpdatedThisFrame = true;

        // 1. Lifetime check
        // 2. Color variation
        // 3. Ignite neighbors
        // 4. Create steam
        
        // Note: Your original fire code did NOT call updateGasMovement, 
        // it just existed until it died. If you want it to rise, call Gas::update at the end.
    }
};
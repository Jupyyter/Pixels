#include "Particles/Gas.hpp"
#include "Particles/Liquid.hpp"
#include "Particles/Solid.hpp"
#include "Constants.hpp"
#include "Random.hpp"
#include <algorithm> 
#include <cmath> 

Gas::Gas(MaterialID id, float buoy, float chaos) 
    : Particle(id), buoyancy(buoy), chaosLevel(chaos) {
    density = 1;
    dispersionRate = 1;
}

// --- CORE MOVEMENT ---
void Gas::update(int x, int y, float dt, ParticleWorld& world) 
{
    world.updateParticleColor(this,world);
    if (hasBeenUpdatedThisFrame) return;
    hasBeenUpdatedThisFrame = true;

    // 1. Calculate Velocity
    velocity.y = std::clamp(velocity.y - (GRAVITY * dt * buoyancy), -5.0f, 2.0f);
    
    // Apply Chaos
    velocity.x += Random::randFloat(-chaosLevel, chaosLevel);
    velocity.x = std::clamp(velocity.x, -3.0f, 3.0f);
    
    // Turbulence
    if (Random::chance(5)) {
        velocity.x += Random::randFloat(-1.0f, 1.0f);
        velocity.y += Random::randFloat(-0.5f, 0.5f);
    }

    int targetX = x + static_cast<int>(std::round(velocity.x));
    int targetY = y + static_cast<int>(std::round(velocity.y));
    
    // Helper lambda using the new actOnNeighbor signature
    auto tryMove = [&](int tx, int ty) -> bool {
        // x and y are passed by reference as currentX and currentY
        return actOnNeighbor(tx, ty, x, y, world, true, true, 0);
    };

    // --- Original 5-Step Movement Logic ---

    // 1. Try Direct Movement
    if (tryMove(targetX, targetY)) {
        // Moved
    }
    // 2. Try Upward Movement
    else if (tryMove(x, y - 1)) {
        // Moved up
    }
    // 3. Try Horizontal Movement (Drift)
    else {
        int direction = (velocity.x > 0) ? 1 : -1;
        if (std::abs(velocity.x) < 0.1f) direction = Random::randBool() ? 1 : -1;

        if (tryMove(x + direction, y)) {
            velocity.x += direction * 0.5f;
        } 
        else if (tryMove(x - direction, y)) {
            velocity.x -= direction * 0.5f;
        }
        // 4. Try Diagonal Upward
        else if (tryMove(x + 1, y - 1)) {
            // Moved diagonal right-up
        }
        else if (tryMove(x - 1, y - 1)) {
            // Moved diagonal left-up
        }
    }

    // 5. Friction
    velocity.x *= 0.8f;
    velocity.y *= 0.9f;

    // --- Side Effects ---
    applyHeatToNeighborsIfIgnited(world);
    spawnSparkIfIgnited(world);
    checkLifeSpan(world);
    takeEffectsDamage(world);
}

// --- INTERACTION LOGIC ---

bool Gas::actOnNeighbor(int targetX, int targetY, int& currentX, int& currentY, ParticleWorld& world, bool isFinal, bool isFirst, int depth) {
    
    if (!world.inBounds(targetX, targetY)) return false;
    Particle* neighbor = world.getParticleAt(targetX, targetY);

    // 1. actOnOther hook
    bool acted = actOnOther(neighbor, world);
    if (acted) return true;

    MaterialGroup nGroup = neighbor->getGroup();
    MaterialID nID = neighbor->id;

    // Logic for Empty or Particle (pass-through)
    if (nID == MaterialID::EmptyParticle) {
        if (isFinal) {
            world.swapParticles(currentX, currentY, targetX, targetY);
            return true; 
        } 
        return false;
    } 
    // Logic for Gas (Density Check)
    else if (nGroup == MaterialGroup::Gas) {
        if (compareGasDensities(neighbor)) {
            swapGasForDensities(world, neighbor, targetX, targetY, currentX, currentY);
            return true; 
        }
        return false;
    }
    // Logic for Liquid (Gases rise through liquids)
    else if (nGroup == MaterialGroup::Liquid) {
        world.swapParticles(currentX, currentY, targetX, targetY);
        return true;
    }
    
    return false;
}

// --- HELPERS ---

bool Gas::compareGasDensities(Particle* neighbor) {
    // Standard logic: Heavier gas sinks. 
    return (density > neighbor->density && neighbor->position.y <= this->position.y);
}

void Gas::swapGasForDensities(ParticleWorld& world, Particle* neighbor, int neighborX, int neighborY, int& currentX, int& currentY) {
    this->velocity.y = 2.0f; // Force push down
    world.swapParticles(currentX, currentY, neighborX, neighborY);
}

// --- SPECIFIC CLASS IMPLEMENTATIONS ---

// STEAM
void Steam::checkLifeSpan(ParticleWorld& world) {
    if (lifeSpan > 0) {
        lifeSpan--;
        if (lifeSpan <= 0) {
            if (Random::randFloat(0, 1) > 0.5f) {
                die(world);
            } else {
                dieAndReplace(MaterialID::Water, world);
            }
        }
    }
}

// SPARK
bool Spark::actOnNeighbor(int targetX, int targetY, int& currentX, int& currentY, ParticleWorld& world, bool isFinal, bool isFirst, int depth) {
    
    if (!world.inBounds(targetX, targetY)) return false;
    Particle* neighbor = world.getParticleAt(targetX, targetY);

    bool acted = actOnOther(neighbor, world);
    if (acted) return true;

    MaterialID nID = neighbor->id;
    MaterialGroup nGroup = neighbor->getGroup();

    if (nID == MaterialID::EmptyParticle) {
        if (isFinal) {
            world.swapParticles(currentX, currentY, targetX, targetY);
        }
        return true; 
    }
    else if (nID == MaterialID::Spark||nID==MaterialID::ExplosionSpark) {
        return false; 
    }
    else if (nID == MaterialID::Smoke) {
        neighbor->die(world);
        return false;
    }
    else if (nGroup == MaterialGroup::Liquid || nGroup == MaterialGroup::MovableSolid||nGroup == MaterialGroup::ImmovableSolid || nGroup == MaterialGroup::Gas) {
        neighbor->receiveHeat(this->heatFactor);
        this->die(world); 
        return true; 
    }
    return false;
}

// EXPLOSION SPARK
bool ExplosionSpark::actOnNeighbor(int targetX, int targetY, int& currentX, int& currentY, ParticleWorld& world, bool isFinal, bool isFirst, int depth) {
    
    if (!world.inBounds(targetX, targetY)) return false;
    Particle* neighbor = world.getParticleAt(targetX, targetY);

    bool acted = actOnOther(neighbor, world);
    if (acted) return true;

    MaterialID nID = neighbor->id;
    MaterialGroup nGroup = neighbor->getGroup();

    if (nID == MaterialID::EmptyParticle) {
        if (isFinal) {
            world.swapParticles(currentX, currentY, targetX, targetY);
        }
        return true;
    }
    else if (nID == MaterialID::Spark||nID==MaterialID::ExplosionSpark) {
        return false;
    }
    else if (nID == MaterialID::Smoke) {
        neighbor->die(world);
        return false;
    }
    else if (nGroup == MaterialGroup::Liquid || nGroup == MaterialGroup::MovableSolid||nGroup == MaterialGroup::ImmovableSolid || nGroup == MaterialGroup::Gas) {
        neighbor->receiveHeat(this->heatFactor);
        this->die(world);
        return true;
    }
    return false;
}